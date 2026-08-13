#include "Quest/LastFPSQuestSubsystem.h"

#include "Cinematics/LastFPSCinematicPlaybackSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Game/LastFPSPlayerController.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "Hub/LastFPSNPCSpawnData.h"
#include "Localization/LastFPSLocalization.h"

#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuest, Log, All);

// ── 목표 유형별 판정 트래커 (구현 세부 — 헤더에 노출하지 않는다) ──────────────────

namespace
{
	/** 아이템 획득 — 수락 시점 보유량 기준선 이후의 증가분(pull형). */
	class FAcquireItemTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual int32 CaptureBaseline(const FLastFPSQuestObjective& Obj, const FLastFPSObjectiveEvalContext& Ctx) const override
		{
			return Ctx.Economy ? Ctx.Economy->GetItemCount(Obj.TargetId) : 0;
		}
		virtual bool RecomputeProgress(
			const FLastFPSQuestObjective& Obj,
			int32 Baseline,
			const FLastFPSObjectiveEvalContext& Ctx,
			int32& OutProgress) const override
		{
			const int32 Current = Ctx.Economy ? Ctx.Economy->GetItemCount(Obj.TargetId) : 0;
			OutProgress = Current - Baseline; // 클램프는 호출부가 일괄 처리
			return true;
		}
	};

	/**
	 * 위치 도달 — 로컬 폰이 도달 지점의 AcceptRadius(m) 이내면 완료(pull형, 이진).
	 * "도달했다"는 사실은 되돌아가지 않으므로 진행은 단조로 유지한다(지점을 벗어나도 재활성 금지).
	 */
	class FReachLocationTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual bool IsProgressMonotonic() const override { return true; }

		virtual bool RecomputeProgress(
			const FLastFPSQuestObjective& Obj,
			int32 Baseline,
			const FLastFPSObjectiveEvalContext& Ctx,
			int32& OutProgress) const override
		{
			OutProgress = 0;
			if (!Ctx.Subsystem)
			{
				return true;
			}

			// 볼륨 트리거 안이거나(정확), 도달 지점의 AcceptRadius 이내면 도달으로 본다.
			bool bReached = Ctx.Subsystem->IsLocationTriggerActive(Obj.TargetTag);
			if (!bReached && Ctx.bHasPlayerLocation)
			{
				FVector Target;
				if (Ctx.Subsystem->ResolveObjectiveLocation(Obj, Target))
				{
					const float RadiusCm = Obj.AcceptRadius * 100.f; // m → cm
					bReached = FVector::DistSquared(Ctx.PlayerLocation, Target) <= FMath::Square(RadiusCm);
				}
			}
			if (bReached)
			{
				OutProgress = Obj.RequiredCount;
			}
			return true;
		}
	};

	/** 대상 처치 — 서버 사망 이벤트를 태그 계층 매칭으로 누적(push형). */
	class FKillTargetTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual bool MatchesEvent(const FLastFPSObjectiveEvent& Event, const FLastFPSQuestObjective& Obj) const override
		{
			return Event.Type == ELastFPSObjectiveType::KillTarget
				&& Obj.TargetTag.IsValid()
				&& Event.Tag.MatchesTag(Obj.TargetTag);
		}
	};

	/** NPC 대화 — 상호작용 NPC 행 이름 일치로 누적(push형). */
	class FTalkToNPCTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual bool MatchesEvent(const FLastFPSObjectiveEvent& Event, const FLastFPSQuestObjective& Obj) const override
		{
			return Event.Type == ELastFPSObjectiveType::TalkToNPC
				&& !Obj.TargetId.IsNone()
				&& Event.Id == Obj.TargetId;
		}
	};

	/** 인카운터 클리어 — ClearEncounter (push형). */
	class FClearEncounterTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual bool MatchesEvent(const FLastFPSObjectiveEvent& Event, const FLastFPSQuestObjective& Obj) const override
		{
			return Event.Type == ELastFPSObjectiveType::ClearEncounter
				&& !Obj.TargetId.IsNone()
				&& Event.Id == Obj.TargetId;
		}
	};

	/**
	 * 태그형 완료 이벤트 — 지정 유형의 push 통지를 목표 태그 계층 매칭으로 누적(push형).
	 * 점령/방어처럼 "분류 태그가 일치하는 외부 완료 통지"는 판정이 동일하고 유형만 다르므로,
	 * 유형별 중복 트래커 대신 이 하나를 유형 인자로 재사용한다.
	 */
	class FTagEventTracker : public ILastFPSObjectiveTracker
	{
	public:
		explicit FTagEventTracker(ELastFPSObjectiveType InEventType) : EventType(InEventType) {}
		virtual bool MatchesEvent(const FLastFPSObjectiveEvent& Event, const FLastFPSQuestObjective& Obj) const override
		{
			return Event.Type == EventType
				&& Obj.TargetTag.IsValid()
				&& Event.Tag.MatchesTag(Obj.TargetTag);
		}
	private:
		ELastFPSObjectiveType EventType;
	};
}

ULastFPSQuestSubsystem* ULastFPSQuestSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>() : nullptr;
}

void ULastFPSQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Economy 를 먼저 초기화(진행 판정/보상 지급이 Economy 를 사용). 시드 아이템도 이 시점엔 이미 채워져 있음.
	Collection.InitializeDependency<ULastFPSEconomySubsystem>();
	Collection.InitializeDependency<ULastFPSGameDataSubsystem>();

	Super::Initialize(Collection);

	BuildTrackers();
	SeedRuntimeStates();
	ProcessQuestTransitions(); // 시드 직후 완료/자동수령/체인 상태를 일관되게 정리
	ValidateReferences();
	UpdateLocationPollTimer();

	// 보유 변동 구독 — AcquireItem 진행 자동 추적. (Economy 시드는 델리게이트를 안 태우므로 위 SeedRuntimeStates 에서 기준선/초기진행을 이미 반영.)
	if (ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		Economy->OnInventoryChanged.AddDynamic(this, &ULastFPSQuestSubsystem::HandleInventoryChanged);
		bInventorySubscribed = true;
		Economy->OnPurchaseCommitted.AddUObject(this, &ULastFPSQuestSubsystem::HandlePurchaseCommitted);
		bPurchaseSubscribed = true;
	}

	OnWorldInitHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(
	this, &ULastFPSQuestSubsystem::HandlePostWorldInitialization);
}

void ULastFPSQuestSubsystem::Deinitialize()
{
	// 등록한 델리게이트 리스트에서 정확히 해제한다 (Initialize 는 OnWorldInitializedActors 에 등록).
	FWorldDelegates::OnWorldInitializedActors.Remove(OnWorldInitHandle);
	UnbindEncounterEvents();
	PendingRadioTransmissions.Reset();
	EncounterMarkers.Reset();

	if (bInventorySubscribed)
	{
		if (ULastFPSEconomySubsystem* Economy = GetEconomy())
		{
			Economy->OnInventoryChanged.RemoveDynamic(this, &ULastFPSQuestSubsystem::HandleInventoryChanged);
		}
		bInventorySubscribed = false;
	}
	if (bPurchaseSubscribed)
	{
		if (ULastFPSEconomySubsystem* Economy = GetEconomy())
		{
			Economy->OnPurchaseCommitted.RemoveAll(this);
		}
		bPurchaseSubscribed = false;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetTimerManager().ClearTimer(LocationPollTimerHandle);
	}

	Super::Deinitialize();
}

ULastFPSEconomySubsystem* ULastFPSQuestSubsystem::GetEconomy() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

const UDataTable* ULastFPSQuestSubsystem::GetEncounterTable() const
{
	UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const ULastFPSRoomEncounterSubsystem* EncounterSubsystem = World
		? World->GetSubsystem<ULastFPSRoomEncounterSubsystem>()
		: nullptr;
	return EncounterSubsystem
		? EncounterSubsystem->GetEncounterTable()
		: nullptr;
}

int32 ULastFPSQuestSubsystem::ResolveObjectiveRequiredCount(
	const FLastFPSQuestObjective& Objective) const
{
	if (Objective.Type != ELastFPSObjectiveType::ClearEncounter || Objective.TargetId.IsNone())
	{
		return FMath::Max(Objective.RequiredCount, 1);
	}

	if (const int32* RuntimeRequiredCount =
		EncounterRequiredCounts.Find(Objective.TargetId))
	{
		return FMath::Max(*RuntimeRequiredCount, 1);
	}

	const UDataTable* Table = GetEncounterTable();
	if (!Table || Table->GetRowStruct() != FLastFPSRoomEncounterData::StaticStruct())
	{
		return FMath::Max(Objective.RequiredCount, 1);
	}

	static const FString Context(TEXT("ULastFPSQuestSubsystem::ResolveObjectiveRequiredCount"));
	if (const FLastFPSRoomEncounterData* Encounter =
		Table->FindRow<FLastFPSRoomEncounterData>(Objective.TargetId, Context, false))
	{
		return FMath::Max(Encounter->GetTotalEnemyCount(), 1);
	}

	return FMath::Max(Objective.RequiredCount, 1);
}

void ULastFPSQuestSubsystem::BindEncounterEvents(UWorld& World)
{
	ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
		World.GetSubsystem<ULastFPSRoomEncounterSubsystem>();
	if (BoundEncounterSubsystem.Get() == EncounterSubsystem)
	{
		return;
	}

	UnbindEncounterEvents();
	if (!EncounterSubsystem)
	{
		UE_LOG(
			LogLastFPSQuest,
			Warning,
			TEXT("[Quest] 월드의 RoomEncounterSubsystem을 찾지 못해 인카운터 목표를 연결하지 못했습니다: %s"),
			*World.GetName());
		return;
	}

	EncounterSubsystem->OnEncounterProgressChanged.AddUniqueDynamic(
		this,
		&ULastFPSQuestSubsystem::NotifyEncounterProgress);
	EncounterSubsystem->OnEncounterCleared.AddUniqueDynamic(
		this,
		&ULastFPSQuestSubsystem::NotifyEncounterCleared);
	BoundEncounterSubsystem = EncounterSubsystem;
}

void ULastFPSQuestSubsystem::UnbindEncounterEvents()
{
	if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem = BoundEncounterSubsystem.Get())
	{
		EncounterSubsystem->OnEncounterProgressChanged.RemoveDynamic(
			this,
			&ULastFPSQuestSubsystem::NotifyEncounterProgress);
		EncounterSubsystem->OnEncounterCleared.RemoveDynamic(
			this,
			&ULastFPSQuestSubsystem::NotifyEncounterCleared);
	}
	BoundEncounterSubsystem.Reset();
}



void ULastFPSQuestSubsystem::BuildTrackers()
{
	Trackers.Add(ELastFPSObjectiveType::AcquireItem, MakeUnique<FAcquireItemTracker>());
	Trackers.Add(ELastFPSObjectiveType::ReachLocation, MakeUnique<FReachLocationTracker>());
	Trackers.Add(ELastFPSObjectiveType::KillTarget, MakeUnique<FKillTargetTracker>());
	Trackers.Add(ELastFPSObjectiveType::TalkToNPC, MakeUnique<FTalkToNPCTracker>());
	Trackers.Add(ELastFPSObjectiveType::ClearEncounter, MakeUnique<FClearEncounterTracker>());
	Trackers.Add(ELastFPSObjectiveType::CaptureZone, MakeUnique<FTagEventTracker>(ELastFPSObjectiveType::CaptureZone));
	Trackers.Add(ELastFPSObjectiveType::DefendZone, MakeUnique<FTagEventTracker>(ELastFPSObjectiveType::DefendZone));
}

const ILastFPSObjectiveTracker* ULastFPSQuestSubsystem::GetTracker(ELastFPSObjectiveType Type) const
{
	const TUniquePtr<ILastFPSObjectiveTracker>* Found = Trackers.Find(Type);
	return Found ? Found->Get() : nullptr;
}

FLastFPSObjectiveEvalContext ULastFPSQuestSubsystem::MakeEvalContext() const
{
	FLastFPSObjectiveEvalContext Ctx;
	Ctx.Economy = GetEconomy();
	Ctx.Subsystem = this;

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const APlayerController* PC = GI->GetFirstLocalPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				Ctx.PlayerLocation = Pawn->GetActorLocation();
				Ctx.bHasPlayerLocation = true;
			}
		}
	}
	return Ctx;
}

const UDataTable* ULastFPSQuestSubsystem::GetQuestTable() const
{
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* Table = GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Quest_Definition) : nullptr;

	// RowStruct가 지정되지 않은 DataTable은 ForeachRow/FindRow 호출 시 엔진 에러를 유발한다.
	if (Table && !Table->GetRowStruct())
	{
		UE_LOG(LogLastFPSQuest, Error,
			TEXT("DT_QuestData에 RowStruct가 지정되지 않았습니다. 에디터에서 Row Structure를 FLastFPSQuestData로 설정하십시오. Path=%s"),
			*Table->GetPathName());
		return nullptr;
	}

	return Table;
}

const FLastFPSQuestData* ULastFPSQuestSubsystem::FindQuest(FName QuestId) const
{
	const UDataTable* Table = GetQuestTable();
	if (!Table || QuestId.IsNone())
	{
		return nullptr;
	}
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::FindQuest"));
	return Table->FindRow<FLastFPSQuestData>(QuestId, Ctx, /*bWarnIfMissing=*/false);
}

FText ULastFPSQuestSubsystem::GetQuestGiverDisplayName(const FName QuestId) const
{
	const FLastFPSQuestData* Quest = FindQuest(QuestId);
	if (!Quest || Quest->QuestGiverNPC.IsNone())
	{
		return FText::GetEmpty();
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance
		? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>()
		: nullptr;
	const UDataTable* NPCTable = GameData
		? GameData->FindTable(LastFPSGameDataTags::Data_Table_NPC_Definition)
		: nullptr;
	if (!NPCTable)
	{
		return FText::FromName(Quest->QuestGiverNPC);
	}

	static const FString Context(TEXT("ULastFPSQuestSubsystem::GetQuestGiverDisplayName"));
	const FLastFPSNPCSpawnData* NPC = NPCTable->FindRow<FLastFPSNPCSpawnData>(
		Quest->QuestGiverNPC,
		Context,
		/*bWarnIfMissing=*/false);
	return NPC && !NPC->DisplayName.IsEmpty()
		? NPC->DisplayName
		: FText::FromName(Quest->QuestGiverNPC);
}

void ULastFPSQuestSubsystem::SeedRuntimeStates()
{
	PendingCompletionRadioQuestIds.Reset();

	const UDataTable* Table = GetQuestTable();
	if (!Table)
	{
		UE_LOG(LogLastFPSQuest, Warning, TEXT("GameDataSet에서 DT_QuestData를 찾지 못해 런타임 상태를 만들 수 없습니다."));
		return;
	}

	// 시드 재계산으로 이미 충족된 목표가 완료 무전을 소급 재생하지 않도록 막는다.
	bSuppressObjectiveRadio = true;

	// 시드로 바로 활성인 퀘스트의 시작 무전. 수락 경로를 타지 않아 재생 지점이 없으므로
	// 여기서 모아 두었다가 시드가 끝난 뒤 한 번에 보낸다.
	TArray<FName> SeededStartRadioIds;

	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::SeedRuntimeStates"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this, &SeededStartRadioIds](const FName& RowName, const FLastFPSQuestData& Row)
		{
			FLastFPSQuestRuntimeState State;
			State.Status = Row.Status; // 초기 시드 상태
			State.Progress.Init(0, Row.Objectives.Num());
			State.Baseline.Init(0, Row.Objectives.Num());
			State.RefundedPurchaseQuantity.Init(0, Row.Objectives.Num());
			State.EligiblePurchaseSpend = 0;

			// 부팅부터 활성(InProgress)인 퀘스트는 지금을 수락 시점으로 보고 기준선/진행을 잡는다.
			if (State.Status == ELastFPSQuestStatus::InProgress)
			{
				CaptureBaseline(Row, State);
				RecomputeProgress(RowName, State, Row);

				// 시드로 바로 진행중인 퀘스트는 수락 경로를 타지 않아 추적이 비어 있다.
				// 추적이 없으면 HUD 트래커도 비므로 테이블 행 순서로 첫 진행중 퀘스트를 기본 추적으로 잡는다.
				if (TrackedQuestId.IsNone())
				{
					TrackedQuestId = RowName;
				}

				SeededStartRadioIds.Append(Row.RadioOnStart);
			}

			RuntimeStates.Add(RowName, MoveTemp(State));
		});

	bSuppressObjectiveRadio = false;

	// 부팅 시점이라 HUD 가 아직 없다. 대기열에 실려 무전 위젯이 생성될 때 재생된다.
	if (!SeededStartRadioIds.IsEmpty())
	{
		TriggerRadioByIds(SeededStartRadioIds);
	}

	// 선행 게이팅 — 시드가 끝나 모든 상태가 존재하는 뒤에 적용.
	// 선행 퀘스트가 아직 Claimed 가 아니면, 시작 전(NotStarted)인 후속을 Locked 로 잠근다.
	for (TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::NotStarted)
		{
			continue;
		}
		const FLastFPSQuestData* Def = FindQuest(Pair.Key);
		if (Def && !Def->PrereqQuestId.IsNone()
			&& GetStatus(Def->PrereqQuestId) != ELastFPSQuestStatus::Claimed)
		{
			Pair.Value.Status = ELastFPSQuestStatus::Locked;
		}
	}

	RebuildNPCMarkerCache();
}

void ULastFPSQuestSubsystem::ValidateReferences() const
{
#if !UE_BUILD_SHIPPING
	const UDataTable* QuestDefinitions = GetQuestTable();
	if (!QuestDefinitions)
	{
		return; // SeedRuntimeStates 에서 이미 경고
	}

	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	const bool bCanValidateItems = Economy && Economy->IsItemTableConfigured();
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance
		? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>()
		: nullptr;
	const UDataTable* NPCDefinitions = GameData
		? GameData->FindTable(LastFPSGameDataTags::Data_Table_NPC_Definition)
		: nullptr;
	const bool bCanValidateNPCs =
		NPCDefinitions
		&& NPCDefinitions->GetRowStruct() == FLastFPSNPCSpawnData::StaticStruct();

	const UDataTable* EncounterDefinitions = GetEncounterTable();
	const bool bCanValidateEncounters =
		EncounterDefinitions
		&& EncounterDefinitions->GetRowStruct() == FLastFPSRoomEncounterData::StaticStruct();
	int32 Broken = 0;
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::ValidateReferences"));
	QuestDefinitions->ForeachRow<FLastFPSQuestData>(Ctx,
		[Economy, bCanValidateItems, NPCDefinitions, bCanValidateNPCs, EncounterDefinitions, bCanValidateEncounters, &Broken](
			const FName& RowName,
			const FLastFPSQuestData& Row)
		{
			if (bCanValidateNPCs
				&& !Row.QuestGiverNPC.IsNone()
				&& !NPCDefinitions->GetRowMap().Contains(Row.QuestGiverNPC))
			{
				++Broken;
				UE_LOG(
					LogLastFPSQuest,
					Error,
					TEXT("[Quest] '%s'의 의뢰인 NPC '%s'가 NPCDataTable에 없습니다."),
					*RowName.ToString(),
					*Row.QuestGiverNPC.ToString());
			}

			for (const FLastFPSQuestObjective& Obj : Row.Objectives)
			{
				if (Obj.Type == ELastFPSObjectiveType::AcquireItem
					&& bCanValidateItems
					&& !Obj.TargetId.IsNone()
					&& !Economy->HasItemDefinition(Obj.TargetId))
				{
					++Broken;
					UE_LOG(LogLastFPSQuest, Error,
						TEXT("[Quest] '%s' 의 목표 아이템 '%s' 가 DT_ItemData 에 없음 — 영원히 진행 불가."),
						*RowName.ToString(), *Obj.TargetId.ToString());
				}

				const bool bReferencesEncounter =
					Obj.Type == ELastFPSObjectiveType::ClearEncounter
					|| (Obj.Type == ELastFPSObjectiveType::ReachLocation && !Obj.TargetId.IsNone());
				if (bReferencesEncounter
					&& bCanValidateEncounters
					&& !EncounterDefinitions->GetRowMap().Contains(Obj.TargetId))
				{
					++Broken;
					UE_LOG(
						LogLastFPSQuest,
						Error,
						TEXT("[Quest] '%s'의 목표 인카운터 '%s'가 EncounterTable에 없습니다."),
						*RowName.ToString(),
						*Obj.TargetId.ToString());
				}
			}

			for (const FLastFPSItemGrant& Grant : Row.Reward.Items)
			{
				if (bCanValidateItems
					&& !Grant.RowId.IsNone()
					&& !Economy->HasItemDefinition(Grant.RowId))
				{
					++Broken;
					UE_LOG(LogLastFPSQuest, Error,
						TEXT("[Quest] '%s' 의 보상 아이템 '%s' 가 DT_ItemData 에 없음 — 지급 시 무시됨."),
						*RowName.ToString(), *Grant.RowId.ToString());
				}
			}
		});

	UE_LOG(LogLastFPSQuest, Log, TEXT("퀘스트 테이블 참조 검증: 깨진 참조 %d건."), Broken);
#endif
}

void ULastFPSQuestSubsystem::CaptureBaseline(const FLastFPSQuestData& Def, FLastFPSQuestRuntimeState& State) const
{
	State.Baseline.Init(0, Def.Objectives.Num());

	const FLastFPSObjectiveEvalContext Ctx = MakeEvalContext();
	for (int32 i = 0; i < Def.Objectives.Num(); ++i)
	{
		const FLastFPSQuestObjective& Obj = Def.Objectives[i];
		if (const ILastFPSObjectiveTracker* Tracker = GetTracker(Obj.Type))
		{
			State.Baseline[i] = Tracker->CaptureBaseline(Obj, Ctx);
		}
	}
}

void ULastFPSQuestSubsystem::HandlePurchaseCommitted(const FLastFPSPurchaseReceipt& Receipt)
{
	if (Receipt.ItemRowId.IsNone() || Receipt.Quantity <= 0 || Receipt.UnitPrice < 0)
	{
		return;
	}

	int32 RemainingQuantity = Receipt.Quantity;
	auto RecordForQuestId = [this, &Receipt, &RemainingQuantity](const FName QuestId)
	{
		if (RemainingQuantity <= 0 || QuestId.IsNone() || !IsQuestInScopeForCurrentMap(QuestId))
		{
			return;
		}

		FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
		const FLastFPSQuestData* Def = FindQuest(QuestId);
		if (!State || !Def)
		{
			return;
		}

		RemainingQuantity -= RecordPurchaseForQuest(
			QuestId,
			*State,
			*Def,
			Receipt,
			RemainingQuantity);
	};

	// 플레이어가 명시적으로 추적한 퀘스트가 같은 아이템을 요구하면 우선 배분한다.
	RecordForQuestId(TrackedQuestId);

	const UDataTable* Table = GetQuestTable();
	if (!Table || RemainingQuantity <= 0)
	{
		return;
	}

	// 나머지는 데이터 테이블 행 순서로 배분해 TMap 순회 순서에 따른 비결정성을 피한다.
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::HandlePurchaseCommitted"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this, &RecordForQuestId](const FName& QuestId, const FLastFPSQuestData&)
		{
			if (QuestId != TrackedQuestId)
			{
				RecordForQuestId(QuestId);
			}
		});
}

int32 ULastFPSQuestSubsystem::RecordPurchaseForQuest(
	const FName QuestId,
	FLastFPSQuestRuntimeState& State,
	const FLastFPSQuestData& Def,
	const FLastFPSPurchaseReceipt& Receipt,
	const int32 AvailableQuantity)
{
	if (State.Status != ELastFPSQuestStatus::InProgress
		|| !Def.Reward.PurchaseRefund.IsEnabled()
		|| AvailableQuantity <= 0)
	{
		return 0;
	}

	if (State.RefundedPurchaseQuantity.Num() != Def.Objectives.Num())
	{
		UE_LOG(
			LogLastFPSQuest,
			Warning,
			TEXT("[Quest] '%s'의 환급 수량 상태가 목표 배열과 달라 초기화합니다."),
			*QuestId.ToString());
		State.RefundedPurchaseQuantity.Init(0, Def.Objectives.Num());
		State.EligiblePurchaseSpend = 0;
	}

	int32 ConsumedQuantity = 0;
	for (int32 ObjectiveIndex = 0;
		ObjectiveIndex < Def.Objectives.Num() && ConsumedQuantity < AvailableQuantity;
		++ObjectiveIndex)
	{
		const FLastFPSQuestObjective& Objective = Def.Objectives[ObjectiveIndex];
		if (Objective.Type != ELastFPSObjectiveType::AcquireItem
			|| Objective.TargetId != Receipt.ItemRowId)
		{
			continue;
		}

		const int32 RequiredQuantity = ResolveObjectiveRequiredCount(Objective);
		const int32 CurrentProgress = State.Progress.IsValidIndex(ObjectiveIndex)
			? State.Progress[ObjectiveIndex]
			: 0;
		const int32 RecordedQuantity = State.RefundedPurchaseQuantity[ObjectiveIndex];
		const int32 EligibleRemaining = FMath::Max(
			0,
			RequiredQuantity - FMath::Max(CurrentProgress, RecordedQuantity));
		const int32 QuantityToRecord = FMath::Min(
			EligibleRemaining,
			AvailableQuantity - ConsumedQuantity);
		if (QuantityToRecord <= 0)
		{
			continue;
		}

		State.RefundedPurchaseQuantity[ObjectiveIndex] += QuantityToRecord;
		const int64 AddedSpend = static_cast<int64>(Receipt.UnitPrice) * QuantityToRecord;
		State.EligiblePurchaseSpend = static_cast<int32>(FMath::Min<int64>(
			static_cast<int64>(MAX_int32),
			static_cast<int64>(State.EligiblePurchaseSpend) + AddedSpend));
		ConsumedQuantity += QuantityToRecord;
	}

	return ConsumedQuantity;
}

bool ULastFPSQuestSubsystem::CheckCompletion(
	const FName QuestId,
	FLastFPSQuestRuntimeState& State,
	const FLastFPSQuestData& Def)
{
	if (State.Status != ELastFPSQuestStatus::InProgress)
	{
		return false;
	}

	// 목표가 없거나 전부 충족 → 완료로 단조 승격.
	for (int32 i = 0; i < Def.Objectives.Num(); ++i)
	{
		const int32 Prog = State.Progress.IsValidIndex(i) ? State.Progress[i] : 0;
		if (Prog < ResolveObjectiveRequiredCount(Def.Objectives[i]))
		{
			return false;
		}
	}

	State.Status = ELastFPSQuestStatus::Completed;

	// 부팅 시드로 이미 충족돼 있던 퀘스트까지 소급 재생되지 않도록 목표 무전과 같은 억제 플래그를 공유한다.
	// 정상 플레이 중 완료된 건만 예약하고, 실제 재생은 보상 팝업이 완전히 닫힌 뒤 수행한다.
	if (!bSuppressObjectiveRadio && !Def.RadioOnComplete.IsEmpty())
	{
		PendingCompletionRadioQuestIds.Add(QuestId);
	}

	return true;
}

bool ULastFPSQuestSubsystem::RecomputeProgress(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def)
{
	// 진행중일 때만 계산 — 완료/수령은 단조라 아래로 되돌리지 않는다(소비되돌림 방지).
	if (State.Status != ELastFPSQuestStatus::InProgress)
	{
		return false;
	}

	State.Progress.SetNum(Def.Objectives.Num());
	State.Baseline.SetNum(Def.Objectives.Num());

	const FLastFPSObjectiveEvalContext Ctx = MakeEvalContext();

	bool bChanged = false;
	for (int32 i = 0; i < Def.Objectives.Num(); ++i)
	{
		const FLastFPSQuestObjective& Obj = Def.Objectives[i];
		const ILastFPSObjectiveTracker* Tracker = GetTracker(Obj.Type);

		// pull형만 재계산 — push형(처치/대화)은 이벤트 누적값을 유지한다.
		int32 NewProgress = 0;
		if (Tracker && Tracker->RecomputeProgress(Obj, State.Baseline[i], Ctx, NewProgress))
		{
			const int32 RequiredCount = ResolveObjectiveRequiredCount(Obj);
			NewProgress = FMath::Clamp(NewProgress, 0, RequiredCount);

			const int32 PreviousProgress = State.Progress[i];
			// 사실형 목표(도달 등)는 재계산으로 낮아지지 않는다 — 지점을 벗어나도 목표가 되살아나지 않게.
			if (Tracker->IsProgressMonotonic())
			{
				NewProgress = FMath::Max(NewProgress, PreviousProgress);
			}
			if (PreviousProgress != NewProgress)
			{
				State.Progress[i] = NewProgress;
				bChanged = true;
				NotifyObjectiveCompleted(Obj, PreviousProgress, NewProgress, RequiredCount);
			}
		}

		if (Def.bSequentialObjectives
			&& State.Progress[i] < ResolveObjectiveRequiredCount(Obj))
		{
			break;
		}
	}

	bChanged |= CheckCompletion(QuestId, State, Def);
	return bChanged;
}

bool ULastFPSQuestSubsystem::RecomputeAllActive()
{
	bool bAnyChanged = false;
	for (TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::InProgress)
		{
			continue;
		}
		if (const FLastFPSQuestData* Def = FindQuest(Pair.Key))
		{
			bAnyChanged |= RecomputeProgress(Pair.Key, Pair.Value, *Def);
		}
	}
	return bAnyChanged;
}

void ULastFPSQuestSubsystem::HandleInventoryChanged()
{
	bool bChanged = RecomputeAllActive();
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::UpdateRouteProgress(const FLastFPSObjectiveEvalContext& Context)
{
	if (!Context.bHasPlayerLocation || ObjectiveRoutes.IsEmpty())
	{
		return;
	}

	// 지금 안내 중인(= 미완료) 위치 목표의 경로만 전진시킨다.
	for (const TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::InProgress)
		{
			continue;
		}
		const FLastFPSQuestData* Def = FindQuest(Pair.Key);
		if (!Def)
		{
			continue;
		}

		for (int32 i = 0; i < Def->Objectives.Num(); ++i)
		{
			const FLastFPSQuestObjective& Obj = Def->Objectives[i];
			const int32 Progress = Pair.Value.Progress.IsValidIndex(i) ? Pair.Value.Progress[i] : 0;
			const int32 RequiredCount = ResolveObjectiveRequiredCount(Obj);
			if (Progress >= RequiredCount)
			{
				continue;
			}

			if (Obj.Type == ELastFPSObjectiveType::ReachLocation)
			{
				AdvanceRouteProgress(Obj.TargetTag, Context.PlayerLocation);
			}

			if (Def->bSequentialObjectives)
			{
				break; // 순차 퀘스트는 첫 미완료 목표까지만 안내한다.
			}
		}
	}
}

void ULastFPSQuestSubsystem::HandleLocationPoll()
{
	UpdateRouteProgress(MakeEvalContext());

	bool bChanged = RecomputeAllActive();
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

bool ULastFPSQuestSubsystem::ApplyObjectiveEventToActive(const FLastFPSObjectiveEvent& Event)
{
	bool bAnyChanged = false;
	for (TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::InProgress)
		{
			continue;
		}
		if (const FLastFPSQuestData* Def = FindQuest(Pair.Key))
		{
			bAnyChanged |= ApplyEventToQuest(Pair.Key, Pair.Value, *Def, Event);
		}
	}
	return bAnyChanged;
}

bool ULastFPSQuestSubsystem::ApplyEventToQuest(
	const FName QuestId,
	FLastFPSQuestRuntimeState& State,
	const FLastFPSQuestData& Def,
	const FLastFPSObjectiveEvent& Event)
{
	State.Progress.SetNum(Def.Objectives.Num());

	bool bChanged = false;
	for (int32 i = 0; i < Def.Objectives.Num(); ++i)
	{
		const FLastFPSQuestObjective& Obj = Def.Objectives[i];
		const int32 RequiredCount = ResolveObjectiveRequiredCount(Obj);
		if (Def.bSequentialObjectives && State.Progress[i] >= RequiredCount)
		{
			continue;
		}

		const ILastFPSObjectiveTracker* Tracker = GetTracker(Obj.Type);
		if (Tracker && Tracker->MatchesEvent(Event, Obj))
		{
			const int32 CurrentProgress = State.Progress[i];
			const int32 CandidateProgress = Event.bSetAbsoluteProgress
				? Event.Count
				: CurrentProgress + Event.Count;
			const int32 NewProgress = FMath::Clamp(
				CandidateProgress,
				0,
				RequiredCount);
			if (NewProgress != CurrentProgress)
			{
				State.Progress[i] = NewProgress;
				bChanged = true;
				NotifyObjectiveCompleted(Obj, CurrentProgress, NewProgress, RequiredCount);
			}
		}

		if (Def.bSequentialObjectives)
		{
			break;
		}
	}

	bChanged |= CheckCompletion(QuestId, State, Def);
	return bChanged;
}

void ULastFPSQuestSubsystem::NotifyObjectiveKill(FGameplayTag EnemyTag)
{
	if (!EnemyTag.IsValid())
	{
		return;
	}
	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::KillTarget;
	Event.Tag = EnemyTag;

	bool bChanged = ApplyObjectiveEventToActive(Event);
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::NotifyEncounterCleared(FName EncounterId)
{
	if (EncounterId.IsNone())
	{
		return;
	}

	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::ClearEncounter;
	Event.Id = EncounterId;
	Event.Count = MAX_int32;
	Event.bSetAbsoluteProgress = true;

	bool bChanged = ApplyObjectiveEventToActive(Event);
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::NotifyTaggedObjective(ELastFPSObjectiveType Type, FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}
	FLastFPSObjectiveEvent Event;
	Event.Type = Type;
	Event.Tag = Tag;

	bool bChanged = ApplyObjectiveEventToActive(Event);
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::NotifyEncounterProgress(
	const FName EncounterId,
	const int32 DefeatedEnemyCount,
	const int32 TotalEnemyCount)
{
	if (EncounterId.IsNone() || TotalEnemyCount < 1 || DefeatedEnemyCount < 0)
	{
		return;
	}

	EncounterRequiredCounts.FindOrAdd(EncounterId) = TotalEnemyCount;

	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::ClearEncounter;
	Event.Id = EncounterId;
	Event.Count = FMath::Min(DefeatedEnemyCount, TotalEnemyCount);
	Event.bSetAbsoluteProgress = true;

	bool bChanged = ApplyObjectiveEventToActive(Event);
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::TriggerRadioTransmission(const FLastFPSRadioTransmissionData& RadioData)
{
	TriggerRadioTransmissions({RadioData});
}

void ULastFPSQuestSubsystem::TriggerRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray)
{
	if (RadioDataArray.IsEmpty())
	{
		return;
	}

	if (OnRadioTransmission.IsBound())
	{
		OnRadioTransmission.Broadcast(RadioDataArray);
		return;
	}

	// HUD가 없는 동안 과거 요청을 계속 쌓으면 새 HUD가 오래된 퀘스트 무전부터 재생한다.
	// 현재 상황을 설명하는 최신 요청 하나만 남기되, 요청 내부의 연속 대사 순서는 보존한다.
	PendingRadioTransmissions = RadioDataArray;
}

void ULastFPSQuestSubsystem::StopAllRadioTransmissions()
{
	PendingRadioTransmissions.Reset();
	
	if (OnRadioTransmissionStop.IsBound())
	{
		OnRadioTransmissionStop.Broadcast();
	}
}

void ULastFPSQuestSubsystem::TriggerRadioByIds(const TArray<FName>& RadioIds)
{
	TArray<FLastFPSRadioTransmissionData> Transmissions;
	Transmissions.Reserve(RadioIds.Num());

	for (const FName& RadioId : RadioIds)
	{
		if (const FLastFPSRadioTransmissionData* Data = FindRadioTransmission(RadioId))
		{
			Transmissions.Add(*Data);
		}
		else
		{
			UE_LOG(LogLastFPSQuest, Warning, TEXT("RadioTransmission 행 '%s' 을 찾을 수 없습니다."), *RadioId.ToString());
		}
	}

	TriggerRadioTransmissions(Transmissions);
}

void ULastFPSQuestSubsystem::FlushPendingRadioTransmissions()
{
	if (!OnRadioTransmission.IsBound() || PendingRadioTransmissions.IsEmpty())
	{
		return;
	}

	TArray<FLastFPSRadioTransmissionData> Pending = MoveTemp(PendingRadioTransmissions);
	PendingRadioTransmissions.Reset();
	OnRadioTransmission.Broadcast(Pending);
}

void ULastFPSQuestSubsystem::HandlePostWorldInitialization(const FActorsInitializedParams& Params)
{
	if (Params.World && Params.World->IsGameWorld())
	{
		// 이전 월드에서 소비되지 못한 무전은 새 전투 레벨에 이어서 재생하지 않는다.
		PendingRadioTransmissions.Reset();
		EncounterRequiredCounts.Reset();
		EncounterMarkers.Reset();
		BindEncounterEvents(*Params.World);
		ScanObjectivePaths(*Params.World);
		AcceptDungeonQuestForMap(*Params.World);
		// 맵이 바뀌면 퀘스트 범위 판정도 달라지므로 마커 표시 캐시를 다시 잡는다.
		RebuildNPCMarkerCache();
	}
}

const UDataTable* ULastFPSQuestSubsystem::GetRadioTable() const
{
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	return GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Radio_Transmission) : nullptr;
}

const FLastFPSRadioTransmissionData* ULastFPSQuestSubsystem::FindRadioTransmission(FName RadioId) const
{
	const UDataTable* Table = GetRadioTable();
	if (!Table || RadioId.IsNone())
	{
		return nullptr;
	}
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::FindRadioTransmission"));
	return Table->FindRow<FLastFPSRadioTransmissionData>(RadioId, Ctx, /*bWarnIfMissing=*/false);
}

void ULastFPSQuestSubsystem::NotifyObjectiveCaptured(FGameplayTag ZoneTag)
{
	NotifyTaggedObjective(ELastFPSObjectiveType::CaptureZone, ZoneTag);
}

void ULastFPSQuestSubsystem::NotifyObjectiveDefended(FGameplayTag ZoneTag)
{
	NotifyTaggedObjective(ELastFPSObjectiveType::DefendZone, ZoneTag);
}

void ULastFPSQuestSubsystem::NotifyTalkedToNPC(FName NPCRowName)
{
	if (NPCRowName.IsNone())
	{
		return;
	}

	// 대화 통지는 이미 진행 중인 목표만 갱신한다. 퀘스트 수락은 NPC 화면의 퀘스트 행 클릭이 담당한다.
	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::TalkToNPC;
	Event.Id = NPCRowName;
	bool bChanged = ApplyObjectiveEventToActive(Event);

	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

bool ULastFPSQuestSubsystem::NotifyTalkedToNPCForQuest(const FName QuestId, const FName NPCRowName)
{
	if (QuestId.IsNone() || NPCRowName.IsNone())
	{
		return false;
	}

	FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	const FLastFPSQuestData* Definition = FindQuest(QuestId);
	if (State == nullptr || Definition == nullptr || State->Status != ELastFPSQuestStatus::InProgress)
	{
		return false;
	}

	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::TalkToNPC;
	Event.Id = NPCRowName;

	bool bChanged = ApplyEventToQuest(QuestId, *State, *Definition, Event);
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
	return bChanged;
}

namespace
{
	/** 같은 컴포넌트가 등록한 항목만 해제한다(늦게 파괴되는 액터가 남의 등록을 지우지 않도록). */
	template<typename KeyType>
	void UnregisterMarkerIn(
		TMap<KeyType, TWeakObjectPtr<USceneComponent>>& Registry,
		const KeyType& Key,
		const USceneComponent* Marker)
	{
		const TWeakObjectPtr<USceneComponent>* Found = Registry.Find(Key);
		if (Found && Found->Get() == Marker)
		{
			Registry.Remove(Key);
		}
	}

	template<typename KeyType>
	bool GetMarkerLocationIn(
		const TMap<KeyType, TWeakObjectPtr<USceneComponent>>& Registry,
		const KeyType& Key,
		FVector& OutLocation)
	{
		if (const TWeakObjectPtr<USceneComponent>* Found = Registry.Find(Key))
		{
			if (const USceneComponent* Marker = Found->Get())
			{
				OutLocation = Marker->GetComponentLocation();
				return true;
			}
		}

		return false;
	}
}

void ULastFPSQuestSubsystem::RegisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker)
{
	if (!LocationTag.IsValid() || !Marker)
	{
		return;
	}
	LocationMarkers.Add(LocationTag, Marker);
	// 마커가 활성 ReachLocation 목표보다 늦게 등장했을 수 있어 폴 타이머를 재평가.
	UpdateLocationPollTimer();
}

void ULastFPSQuestSubsystem::UnregisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker)
{
	UnregisterMarkerIn(LocationMarkers, LocationTag, Marker);
}

bool ULastFPSQuestSubsystem::GetTrackedLocation(FGameplayTag LocationTag, FVector& OutLocation) const
{
	return GetMarkerLocationIn(LocationMarkers, LocationTag, OutLocation);
}

void ULastFPSQuestSubsystem::RegisterEncounterMarker(
	const FName EncounterId,
	USceneComponent* Marker)
{
	if (EncounterId.IsNone() || !Marker)
	{
		return;
	}

	EncounterMarkers.Add(EncounterId, Marker);
}

void ULastFPSQuestSubsystem::UnregisterEncounterMarker(
	const FName EncounterId,
	USceneComponent* Marker)
{
	UnregisterMarkerIn(EncounterMarkers, EncounterId, Marker);
}

bool ULastFPSQuestSubsystem::GetTrackedEncounterLocation(
	const FName EncounterId,
	FVector& OutLocation) const
{
	return GetMarkerLocationIn(EncounterMarkers, EncounterId, OutLocation);
}

const FLastFPSQuestData* ULastFPSQuestSubsystem::GetTrackedQuestForDisplay(
	FName& OutQuestId,
	const FLastFPSQuestRuntimeState*& OutState) const
{
	OutState = nullptr;
	OutQuestId = NAME_None;

	// 던전 맵(배틀 맵)에 있다면, 사용자가 다른 퀘스트를 추적했더라도 무조건 던전 퀘스트를 우선 표시한다.
	for (const FName DungeonQuestId : CurrentMapQuestIds)
	{
		const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(DungeonQuestId);
		const FLastFPSQuestData* Def = FindQuest(DungeonQuestId);
		if (State && Def && State->Status == ELastFPSQuestStatus::InProgress)
		{
			OutState = State;
			OutQuestId = DungeonQuestId;
			return Def;
		}
	}

	// 그 외의 경우(허브 등)에는 사용자가 명시적으로 지정한 추적 퀘스트를 표시한다.
	// 맵 스코프(IsQuestInScopeForCurrentMap)와 무관하게, 명시적 추적 대상이면 완료/미시작 상태라도 무조건 표시한다.
	if (!TrackedQuestId.IsNone())
	{
		const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(TrackedQuestId);
		const FLastFPSQuestData* Def = FindQuest(TrackedQuestId);
		if (State && Def && State->Status != ELastFPSQuestStatus::Claimed && State->Status != ELastFPSQuestStatus::Locked)
		{
			OutState = State;
			OutQuestId = TrackedQuestId;
			return Def;
		}
	}

	// 만약 추적 중인 퀘스트가 없거나 이미 완료되었다면, 진행 중인 아무 퀘스트나 하나 찾아서 우선 표시한다.
	// 1차: 현재 맵 스코프에 맞는 퀘스트 우선
	for (const auto& Pair : RuntimeStates)
	{
		if ((Pair.Value.Status == ELastFPSQuestStatus::InProgress || Pair.Value.Status == ELastFPSQuestStatus::Completed) && IsQuestInScopeForCurrentMap(Pair.Key))
		{
			const FLastFPSQuestData* Def = FindQuest(Pair.Key);
			if (Def)
			{
				OutState = &Pair.Value;
				OutQuestId = Pair.Key;
				return Def;
			}
		}
	}

	// 2차: 맵 스코프에 맞지 않더라도 진행 중인 퀘스트가 있다면 표시한다.
	for (const auto& Pair : RuntimeStates)
	{
		if (Pair.Value.Status == ELastFPSQuestStatus::InProgress || Pair.Value.Status == ELastFPSQuestStatus::Completed)
		{
			const FLastFPSQuestData* Def = FindQuest(Pair.Key);
			if (Def)
			{
				OutState = &Pair.Value;
				OutQuestId = Pair.Key;
				return Def;
			}
		}
	}

	// 3차: 진행 중인 퀘스트가 하나도 없다면, 아직 수락 전(NotStarted)인 메인 퀘스트라도 우선 표시하여 다음 행동(NPC 대화 등)을 안내한다.
	for (const auto& Pair : RuntimeStates)
	{
		if (Pair.Value.Status == ELastFPSQuestStatus::NotStarted)
		{
			const FLastFPSQuestData* Def = FindQuest(Pair.Key);
			if (Def && Def->Type == ELastFPSQuestType::Main)
			{
				OutState = &Pair.Value;
				OutQuestId = Pair.Key;
				return Def;
			}
		}
	}

	return nullptr;
}

void ULastFPSQuestSubsystem::ForEachDisplayObjective(
	const FLastFPSQuestData& Def,
	const FLastFPSQuestRuntimeState& State,
	TFunctionRef<bool(const FLastFPSDisplayObjectiveEntry&)> Visitor) const
{
	for (int32 ObjectiveIndex = 0; ObjectiveIndex < Def.Objectives.Num(); ++ObjectiveIndex)
	{
		FLastFPSDisplayObjectiveEntry Entry;
		Entry.Objective = &Def.Objectives[ObjectiveIndex];
		Entry.Index = ObjectiveIndex;
		Entry.Progress = State.Progress.IsValidIndex(ObjectiveIndex) ? State.Progress[ObjectiveIndex] : 0;
		Entry.RequiredCount = ResolveObjectiveRequiredCount(*Entry.Objective);
		Entry.bCompleted = Entry.Progress >= Entry.RequiredCount;

		if (!Visitor(Entry))
		{
			return;
		}

		// 순차 퀘스트는 첫 미완료 목표가 곧 현재 단계다. 그 뒤는 아직 공개하지 않는다.
		if (Def.bSequentialObjectives && !Entry.bCompleted)
		{
			return;
		}
	}
}

void ULastFPSQuestSubsystem::TriggerDialogueAsRadio(
	const FLastFPSDialogueData& Dialogue,
	const FText& FallbackSpeaker,
	const FLinearColor SpeakerColor)
{
	if (Dialogue.Lines.IsEmpty())
	{
		return;
	}

	const FText Speaker = Dialogue.SpeakerName.IsEmpty() ? FallbackSpeaker : Dialogue.SpeakerName;

	TArray<FLastFPSRadioTransmissionData> Transmissions;
	Transmissions.Reserve(Dialogue.Lines.Num());
	for (const FText& Line : Dialogue.Lines)
	{
		if (Line.IsEmpty())
		{
			continue;
		}

		FLastFPSRadioTransmissionData& Entry = Transmissions.AddDefaulted_GetRef();
		Entry.SpeakerName = Speaker;
		Entry.SpeakerColor = SpeakerColor;
		Entry.DialogueText = Line;
	}

	TriggerRadioTransmissions(Transmissions);
}

void ULastFPSQuestSubsystem::GetNPCQuestActions(
	const FName NPCRowName,
	TArray<FName>& OutAcceptable,
	TArray<FName>& OutReportable) const
{
	OutAcceptable.Reset();
	OutReportable.Reset();

	const UDataTable* Table = GetQuestTable();
	if (Table == nullptr || NPCRowName.IsNone())
	{
		return;
	}

	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::GetNPCQuestActions"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this, NPCRowName, &OutAcceptable, &OutReportable](const FName& RowName, const FLastFPSQuestData& Row)
		{
			if (!IsQuestInScopeForCurrentMap(RowName))
			{
				return;
			}

			const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(RowName);
			if (State == nullptr)
			{
				return;
			}

			// 잠긴 퀘스트는 아직 존재를 드러내지 않는다(마커 규칙과 동일).
			if (State->Status == ELastFPSQuestStatus::NotStarted && Row.QuestGiverNPC == NPCRowName)
			{
				OutAcceptable.Add(RowName);
				return;
			}

			if (State->Status != ELastFPSQuestStatus::InProgress)
			{
				return;
			}

			ForEachDisplayObjective(Row, *State,
				[NPCRowName, &OutReportable, &RowName](const FLastFPSDisplayObjectiveEntry& Entry)
				{
					if (!Entry.bCompleted
						&& Entry.Objective->Type == ELastFPSObjectiveType::TalkToNPC
						&& Entry.Objective->TargetId == NPCRowName)
					{
						OutReportable.Add(RowName);
						return false;
					}

					return true;
				});
		});
}

void ULastFPSQuestSubsystem::GetNPCQuestOptions(
	const FName NPCRowName,
	TArray<FLastFPSNPCQuestOption>& OutOptions) const
{
	OutOptions.Reset();

	TArray<FName> Acceptable;
	TArray<FName> Reportable;
	GetNPCQuestActions(NPCRowName, Acceptable, Reportable);
	OutOptions.Reserve(Acceptable.Num() + Reportable.Num() + 1);

	auto AddOption = [this, &OutOptions](const FName QuestId, const ELastFPSNPCQuestOptionType Type)
	{
		const FLastFPSQuestData* Definition = FindQuest(QuestId);
		if (Definition == nullptr)
		{
			return;
		}

		FLastFPSNPCQuestOption& Option = OutOptions.AddDefaulted_GetRef();
		Option.QuestId = QuestId;
		Option.Title = Definition->Title;
		Option.Content = Definition->NPCButtonTextKey.IsNone()
			? (Definition->Summary.IsEmpty() ? Definition->Title : Definition->Summary)
			: FLastFPSLocalization::GetUIText(Definition->NPCButtonTextKey);
		Option.Description = Definition->Description;
		Option.Type = Type;
	};

	for (const FName QuestId : Acceptable)
	{
		AddOption(QuestId, ELastFPSNPCQuestOptionType::Accept);
	}
	for (const FName QuestId : Reportable)
	{
		AddOption(QuestId, ELastFPSNPCQuestOptionType::Report);
	}

	const UDataTable* Table = GetQuestTable();
	if (Table == nullptr)
	{
		return;
	}

	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::GetNPCQuestOptions.Guidance"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this, NPCRowName, &OutOptions](const FName& QuestId, const FLastFPSQuestData& Row)
		{
			const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
			if (State == nullptr
				|| State->Status != ELastFPSQuestStatus::InProgress
				|| !IsQuestInScopeForCurrentMap(QuestId))
			{
				return;
			}

			ForEachDisplayObjective(Row, *State,
				[this, NPCRowName, QuestId, &Row, &OutOptions](const FLastFPSDisplayObjectiveEntry& Entry)
				{
					if (Entry.bCompleted
						|| Entry.Objective->GuidanceNPCRowName != NPCRowName
						|| !Entry.Objective->GuidanceScreenTag.IsValid())
					{
						return true;
					}

					FLastFPSNPCQuestOption& Option = OutOptions.AddDefaulted_GetRef();
					Option.QuestId = QuestId;
					Option.Title = Row.Title;
					Option.Content = Row.NPCButtonTextKey.IsNone()
						? (Row.Summary.IsEmpty() ? Row.Title : Row.Summary)
						: FLastFPSLocalization::GetUIText(Row.NPCButtonTextKey);
					Option.Description = Row.Description;
					Option.Type = ELastFPSNPCQuestOptionType::OpenGuidanceScreen;
					Option.ScreenTag = Entry.Objective->GuidanceScreenTag;
					return false;
				});
		});
}

FLastFPSQuestNPCMarkerInfo ULastFPSQuestSubsystem::GetNPCMarkerInfo(const FName NPCRowName) const
{
	if (const FLastFPSQuestNPCMarkerInfo* Found = NPCMarkerInfos.Find(NPCRowName))
	{
		return *Found;
	}

	return FLastFPSQuestNPCMarkerInfo();
}

void ULastFPSQuestSubsystem::RebuildNPCMarkerCache()
{
	NPCMarkerInfos.Reset();

	const UDataTable* Table = GetQuestTable();
	if (!Table)
	{
		return;
	}

	// 진행중 대화 목표가 수락 가능 표시보다 우선한다(지금 해야 할 일을 먼저 보여준다).
	auto ApplyMarker =
		[this](const FName NPCRowName, const ELastFPSQuestNPCMarkerSymbol Symbol, const bool bTracked)
		{
			if (NPCRowName.IsNone())
			{
				return;
			}

			FLastFPSQuestNPCMarkerInfo& Info = NPCMarkerInfos.FindOrAdd(NPCRowName);
			if (Info.Symbol == ELastFPSQuestNPCMarkerSymbol::Objective
				&& Symbol != ELastFPSQuestNPCMarkerSymbol::Objective)
			{
				// 이미 더 우선하는 심볼이 잡혀 있으면 강조 여부만 누적한다.
				Info.bTracked |= bTracked;
				return;
			}
			if (Info.Symbol == ELastFPSQuestNPCMarkerSymbol::Available
				&& Symbol == ELastFPSQuestNPCMarkerSymbol::InProgress)
			{
				// Available(느낌표)가 InProgress(진행 중 모래시계 등)보다 우선한다.
				Info.bTracked |= bTracked;
				return;
			}

			Info.Symbol = Symbol;
			Info.bTracked |= bTracked;
		};

	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::RebuildNPCMarkerCache"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this, &ApplyMarker](const FName& RowName, const FLastFPSQuestData& Row)
		{
			if (!IsQuestInScopeForCurrentMap(RowName))
			{
				return;
			}

			const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(RowName);
			if (!State)
			{
				return;
			}

			const bool bTracked = (RowName == TrackedQuestId);

			// 아직 수락하지 않은(잠기지도 않은) 퀘스트를 주는 NPC 에는 수락 가능 심볼을 띄운다.
			if (State->Status == ELastFPSQuestStatus::NotStarted)
			{
				ApplyMarker(Row.QuestGiverNPC, ELastFPSQuestNPCMarkerSymbol::Available, bTracked);
				return;
			}

			if (State->Status != ELastFPSQuestStatus::InProgress)
			{
				return;
			}

			// 진행중 퀘스트의 활성 NPC 안내 목표 — 순차 규칙은 공용 순회가 소유한다.
			bool bHasNPCTarget = false;
			ForEachDisplayObjective(Row, *State,
				[&ApplyMarker, &bHasNPCTarget, bTracked](const FLastFPSDisplayObjectiveEntry& Entry)
				{
					if (Entry.bCompleted)
					{
						return true;
					}

					if (Entry.Objective->Type == ELastFPSObjectiveType::TalkToNPC)
					{
						ApplyMarker(Entry.Objective->TargetId, ELastFPSQuestNPCMarkerSymbol::Objective, bTracked);
						bHasNPCTarget = true;
					}
					else if (!Entry.Objective->GuidanceNPCRowName.IsNone())
					{
						ApplyMarker(Entry.Objective->GuidanceNPCRowName, ELastFPSQuestNPCMarkerSymbol::Objective, bTracked);
						bHasNPCTarget = true;
					}

					return true;
				});

			// 대화 목표가 없는 퀘스트(아이템 획득/처치 등)는 가리킬 NPC 가 사라져
			// 추적해도 마커가 하나도 남지 않는다. 그런 경우 의뢰한 NPC 를 진행 중 대상으로 남긴다.
			if (!bHasNPCTarget)
			{
				ApplyMarker(Row.QuestGiverNPC, ELastFPSQuestNPCMarkerSymbol::InProgress, bTracked);
			}
		});
}

bool ULastFPSQuestSubsystem::ResolveObjectiveLocation(
	const FLastFPSQuestObjective& Objective,
	FVector& OutLocation) const
{
	// 레벨에 마커 컴포넌트를 붙인 대상(이동하는 NPC 등)이 우선한다.
	if (Objective.TargetTag.IsValid() && GetTrackedLocation(Objective.TargetTag, OutLocation))
	{
		return true;
	}

	// 그 외에는 같은 태그로 배치된 이동 동선의 마지막 지점이 도달 지점이다.
	return GetRouteDestination(Objective.TargetTag, OutLocation);
}

void ULastFPSQuestSubsystem::NotifyLocationTriggerChanged(FGameplayTag LocationTag, bool bPlayerInside)
{
	if (!LocationTag.IsValid())
	{
		return;
	}

	int32& Count = LocationTriggerOverlaps.FindOrAdd(LocationTag);
	if (bPlayerInside)
	{
		++Count;
	}
	else
	{
		Count = FMath::Max(0, Count - 1);
		if (Count == 0)
		{
			LocationTriggerOverlaps.Remove(LocationTag);
		}
	}

	// 진입 즉시 반영 — 폴 주기를 기다리지 않는다. (퇴장은 완료 전이가 단조라 무해)
	bool bChanged = RecomputeAllActive();
	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
	}
}

bool ULastFPSQuestSubsystem::IsLocationTriggerActive(FGameplayTag LocationTag) const
{
	const int32* Count = LocationTriggerOverlaps.Find(LocationTag);
	return Count && *Count > 0;
}

void ULastFPSQuestSubsystem::GetActiveWaypoints(TArray<FLastFPSObjectiveWaypoint>& OutWaypoints) const
{
	OutWaypoints.Reset();

	const FLastFPSQuestRuntimeState* State = nullptr;
	FName DisplayQuestId = NAME_None;
	const FLastFPSQuestData* Def = GetTrackedQuestForDisplay(DisplayQuestId, State);
	if (!Def || !State)
	{
		return;
	}

	// 안내 지점의 전진은 위치 폴이 담당하고, 여기서는 현재 지점을 읽기만 한다.
	ForEachDisplayObjective(*Def, *State,
		[this, Def, &OutWaypoints](const FLastFPSDisplayObjectiveEntry& Entry)
		{
			// 이미 완료된 목표는 마커를 띄우지 않는다.
			if (Entry.bCompleted)
			{
				return true;
			}

			// 안내 위치가 해석되는 목표만 웨이포인트가 된다(유형 판정은 해석기가 소유).
			FLastFPSObjectiveGuidance Guidance;
			if (!ResolveObjectiveGuidanceLocation(*Entry.Objective, Guidance))
			{
				return true;
			}

			FLastFPSObjectiveWaypoint& Waypoint = OutWaypoints.AddDefaulted_GetRef();
			Waypoint.WorldLocation = Guidance.Location;
			Waypoint.Label = Entry.Objective->Label.IsEmpty() ? Def->Title : Entry.Objective->Label;
			Waypoint.QuestId = TrackedQuestId;
			Waypoint.LocationTag = Entry.Objective->TargetTag;
			Waypoint.bIsRoutePoint = !Guidance.bIsDestination;
			return true;
		});
}

bool ULastFPSQuestSubsystem::ResolveObjectiveGuidanceLocation(
	const FLastFPSQuestObjective& Objective,
	FLastFPSObjectiveGuidance& OutGuidance) const
{
	OutGuidance = FLastFPSObjectiveGuidance();

	if (Objective.Type == ELastFPSObjectiveType::ClearEncounter)
	{
		return GetTrackedEncounterLocation(Objective.TargetId, OutGuidance.Location);
	}

	// 위치를 해석할 수 있는 유형은 인카운터와 위치 도달뿐이다.
	// 대화 목표는 NPC 머리 위 마커가 전담하므로 여기서 위치를 내주지 않는다
	// (같은 대상을 화면 마커와 머리 위 마커가 동시에 가리키지 않게 하기 위함).
	if (Objective.Type != ELastFPSObjectiveType::ReachLocation)
	{
		return false;
	}

	// 동선이 배치돼 있으면 지금 안내 중인 지점, 없으면 도달 지점을 가리킨다.
	if (GetCurrentRoutePoint(Objective.TargetTag, OutGuidance.Location, OutGuidance.bIsDestination))
	{
		return true;
	}

	OutGuidance.bIsDestination = true;
	return ResolveObjectiveLocation(Objective, OutGuidance.Location);
}

void ULastFPSQuestSubsystem::GetTrackedQuests(TArray<FLastFPSTrackedQuest>& OutQuests) const
{
	OutQuests.Reset();

	// 추적은 동시에 1건이므로 전체 순회 없이 추적 대상만 조회한다(반환형은 기존 계약대로 배열 유지).
	const FLastFPSQuestRuntimeState* State = nullptr;
	FName DisplayQuestId = NAME_None;
	const FLastFPSQuestData* Row = GetTrackedQuestForDisplay(DisplayQuestId, State);
	if (!Row || !State)
	{
		return;
	}

	FLastFPSTrackedQuest& Tracked = OutQuests.AddDefaulted_GetRef();
	Tracked.QuestId = DisplayQuestId;
	Tracked.Title = Row->Title;
	Tracked.Type = Row->Type;

	// 미수락 메인 퀘스트는 실제 목표를 미리 노출하지 않고, 수락을 위해 찾아갈 의뢰인만 안내한다.
	// NPC 머리 위 마커가 위치 안내를 전담하므로 트래커 스냅샷에는 월드 좌표를 중복 제공하지 않는다.
	if (State->Status == ELastFPSQuestStatus::NotStarted && !Row->QuestGiverNPC.IsNone())
	{
		FLastFPSTrackedObjective& Snapshot = Tracked.Objectives.AddDefaulted_GetRef();
		Snapshot.Label = FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestTrackerGoToNPCFormat),
			GetQuestGiverDisplayName(DisplayQuestId));
		Snapshot.Type = ELastFPSObjectiveType::TalkToNPC;
		Snapshot.Progress = 0;
		Snapshot.RequiredCount = 1;
		Snapshot.bCompleted = false;
		Snapshot.bHasGuidanceLocation = false;
		return;
	}

	Tracked.Objectives.Reserve(Row->Objectives.Num());

	ForEachDisplayObjective(*Row, *State,
		[this, &Tracked](const FLastFPSDisplayObjectiveEntry& Entry)
		{
			FLastFPSTrackedObjective& Snapshot = Tracked.Objectives.AddDefaulted_GetRef();
			Snapshot.Label = Entry.Objective->Label;
			Snapshot.Type = Entry.Objective->Type;
			Snapshot.Progress = Entry.Progress;
			Snapshot.RequiredCount = Entry.RequiredCount;
			Snapshot.bCompleted = Entry.bCompleted;

			FLastFPSObjectiveGuidance Guidance;
			Snapshot.bHasGuidanceLocation = !Entry.bCompleted
				&& ResolveObjectiveGuidanceLocation(*Entry.Objective, Guidance);
			Snapshot.GuidanceLocation = Guidance.Location;
			return true;
		});
}

void ULastFPSQuestSubsystem::BroadcastStateChanged()
{
	UpdateLocationPollTimer();
	// 마커 표시 판정은 여기서 한 번만 하고, 구독자(NPC 마커)는 그 결과만 읽는다.
	RebuildNPCMarkerCache();
	OnQuestStateChanged.Broadcast();
}

void ULastFPSQuestSubsystem::UpdateLocationPollTimer()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	bool bNeedPoll = false;
	for (const TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::InProgress)
		{
			continue;
		}
		const FLastFPSQuestData* Def = FindQuest(Pair.Key);
		if (!Def)
		{
			continue;
		}
		for (int32 ObjectiveIndex = 0; ObjectiveIndex < Def->Objectives.Num(); ++ObjectiveIndex)
		{
			const FLastFPSQuestObjective& Obj = Def->Objectives[ObjectiveIndex];
			const int32 Progress = Pair.Value.Progress.IsValidIndex(ObjectiveIndex)
				? Pair.Value.Progress[ObjectiveIndex]
				: 0;
			if (Progress >= ResolveObjectiveRequiredCount(Obj))
			{
				continue;
			}

			if (Obj.Type == ELastFPSObjectiveType::ReachLocation)
			{
				bNeedPoll = true;
				break;
			}
			if (Def->bSequentialObjectives)
			{
				break;
			}
		}
		if (bNeedPoll)
		{
			break;
		}
	}

	FTimerManager& TM = GI->GetTimerManager();
	const bool bActive = TM.IsTimerActive(LocationPollTimerHandle);
	if (bNeedPoll && !bActive)
	{
		TM.SetTimer(LocationPollTimerHandle, this, &ULastFPSQuestSubsystem::HandleLocationPoll, LocationPollInterval, /*bLoop=*/true);
	}
	else if (!bNeedPoll && bActive)
	{
		TM.ClearTimer(LocationPollTimerHandle);
	}
}

ELastFPSQuestStatus ULastFPSQuestSubsystem::GetStatus(FName QuestId) const
{
	const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	return State ? State->Status : ELastFPSQuestStatus::NotStarted;
}

int32 ULastFPSQuestSubsystem::GetObjectiveProgress(FName QuestId, int32 ObjectiveIndex) const
{
	const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	return (State && State->Progress.IsValidIndex(ObjectiveIndex)) ? State->Progress[ObjectiveIndex] : 0;
}

int32 ULastFPSQuestSubsystem::GetObjectiveRequiredCount(
	const FName QuestId,
	const int32 ObjectiveIndex) const
{
	const FLastFPSQuestData* Definition = FindQuest(QuestId);
	if (!Definition || !Definition->Objectives.IsValidIndex(ObjectiveIndex))
	{
		return 0;
	}

	return ResolveObjectiveRequiredCount(Definition->Objectives[ObjectiveIndex]);
}

bool ULastFPSQuestSubsystem::IsComplete(FName QuestId) const
{
	const ELastFPSQuestStatus S = GetStatus(QuestId);
	return S == ELastFPSQuestStatus::Completed || S == ELastFPSQuestStatus::Claimed;
}

bool ULastFPSQuestSubsystem::IsClaimable(FName QuestId) const
{
	return GetStatus(QuestId) == ELastFPSQuestStatus::Completed;
}

bool ULastFPSQuestSubsystem::AcceptQuest(FName QuestId)
{
	FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	const FLastFPSQuestData* Def = FindQuest(QuestId);
	// NotStarted 에서만 수락 — Locked(선행 미충족)/진행중/완료는 거부.
	if (!State || !Def || State->Status != ELastFPSQuestStatus::NotStarted)
	{
		return false;
	}

	AcceptQuestInternal(QuestId, *State, *Def);
	ProcessQuestTransitions(); // 수락 즉시 완료→자동수령→다음 등 연쇄
	BroadcastStateChanged();
	return true;
}

void ULastFPSQuestSubsystem::CancelQuest(FName QuestId)
{
	FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	if (!State || State->Status != ELastFPSQuestStatus::InProgress)
	{
		return;
	}

	State->Status = ELastFPSQuestStatus::NotStarted;
	State->Progress.Empty();
	State->Baseline.Empty();
	ClearTrackedQuestIfMatches(QuestId);

	BroadcastStateChanged();
}

void ULastFPSQuestSubsystem::SetQuestTracked(FName QuestId, bool bTrack)
{
	if (QuestId.IsNone())
	{
		return;
	}

	if (bTrack)
	{
		// 진행중이 아닌 퀘스트는 안내할 목표가 없으므로 추적 대상이 될 수 없다.
		if (GetStatus(QuestId) != ELastFPSQuestStatus::InProgress || TrackedQuestId == QuestId)
		{
			return;
		}

		// 교체(이전 추적 해제 + 신규 추적)를 한 번의 통지로 끝낸다.
		TrackedQuestId = QuestId;
		BroadcastStateChanged();
		return;
	}

	// 이미 다른 퀘스트를 추적 중이면 남의 추적을 끄지 않는다(해제 규칙은 한 곳에만 둔다).
	if (ClearTrackedQuestIfMatches(QuestId))
	{
		BroadcastStateChanged();
	}
}

bool ULastFPSQuestSubsystem::IsQuestTracked(FName QuestId) const
{
	return !QuestId.IsNone() && TrackedQuestId == QuestId;
}

bool ULastFPSQuestSubsystem::ClearTrackedQuestIfMatches(const FName QuestId)
{
	if (QuestId.IsNone() || TrackedQuestId != QuestId)
	{
		return false;
	}

	TrackedQuestId = NAME_None;
	return true;
}

bool ULastFPSQuestSubsystem::AcceptQuestInternal(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def)
{
	State.Status = ELastFPSQuestStatus::InProgress;
	State.Progress.Init(0, Def.Objectives.Num());
	State.RefundedPurchaseQuantity.Init(0, Def.Objectives.Num());
	State.EligiblePurchaseSpend = 0;

	// 추적은 동시에 1건이라 수락이 기존 추적을 빼앗지 않는다. 빈자리일 때만 이어받는다.
	if (TrackedQuestId.IsNone())
	{
		TrackedQuestId = QuestId;
	}

	CaptureBaseline(Def, State);
	RecomputeProgress(QuestId, State, Def); // 수락 즉시 충족되는 경우(목표 0개 등) 반영

	if (!Def.RadioOnStart.IsEmpty())
	{
		TriggerRadioByIds(Def.RadioOnStart);
	}

	// 재생 가능 여부(맵 일치·중복 재생)는 재생 서브시스템이 판단하므로 여기서는 요청만 넘긴다.
	if (Def.CinematicOnStart.IsValidRequest())
	{
		UGameInstance* GameInstance = GetGameInstance();
		UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
		if (ULastFPSCinematicPlaybackSubsystem* Cinematics =
			World ? World->GetSubsystem<ULastFPSCinematicPlaybackSubsystem>() : nullptr)
		{
			Cinematics->RequestPlayback(Def.CinematicOnStart);
		}
	}

	return true;
}

bool ULastFPSQuestSubsystem::TryClaimReward(FName QuestId)
{
	FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	const FLastFPSQuestData* Def = FindQuest(QuestId);
	if (!State || !Def || State->Status != ELastFPSQuestStatus::Completed)
	{
		return false; // 완료 상태에서만 1회 — Claimed 면 재지급 안 함
	}

	// 단조 래치: 지급 전에 상태를 먼저 올려 재진입 시 중복지급을 원천 차단.
	State->Status = ELastFPSQuestStatus::Claimed;
	// 추적 자리를 먼저 비워야 아래 전이에서 자동 수락되는 다음 퀘스트가 추적을 승계한다.
	ClearTrackedQuestIfMatches(QuestId);
	GrantReward(QuestId, *Def);
	BroadcastStateChanged();
	return true;
}

#if !UE_BUILD_SHIPPING
void ULastFPSQuestSubsystem::DebugUnlockChainTo(const FName QuestId)
{
	if (!FindQuest(QuestId))
	{
		UE_LOG(LogLastFPSQuest, Warning, TEXT("DebugUnlockChainTo: 퀘스트 '%s' 를 찾을 수 없습니다."), *QuestId.ToString());
		return;
	}

	// 선행 사슬을 먼저 모은다. 데이터가 순환하면 무한 루프가 되므로 방문 집합으로 끊는다.
	TArray<FName> Chain;
	TSet<FName> Visited;
	for (FName Current = QuestId; !Current.IsNone() && !Visited.Contains(Current); )
	{
		Visited.Add(Current);
		Chain.Add(Current);

		const FLastFPSQuestData* Def = FindQuest(Current);
		Current = Def ? Def->PrereqQuestId : NAME_None;
	}

	// 앞선 퀘스트부터 수령 완료로 올린다. 대상 자신은 수락 가능한 상태까지만 연다.
	for (int32 Index = Chain.Num() - 1; Index >= 0; --Index)
	{
		const FName ChainQuestId = Chain[Index];
		FLastFPSQuestRuntimeState* State = RuntimeStates.Find(ChainQuestId);
		const FLastFPSQuestData* Def = FindQuest(ChainQuestId);
		if (!State || !Def)
		{
			continue;
		}

		const bool bIsTarget = ChainQuestId == QuestId;
		if (bIsTarget)
		{
			if (State->Status == ELastFPSQuestStatus::Locked)
			{
				State->Status = ELastFPSQuestStatus::NotStarted;
			}
			continue;
		}

		State->Status = ELastFPSQuestStatus::Claimed;
		State->Progress.Init(0, Def->Objectives.Num());
		// 완료 무전 예약이 남아 있으면 치트로 건너뛴 퀘스트의 대사가 뒤늦게 재생된다.
		PendingCompletionRadioQuestIds.Remove(ChainQuestId);
	}

	ClearTrackedQuestIfMatches(QuestId);
	ProcessQuestTransitions();
	BroadcastStateChanged();

	UE_LOG(LogLastFPSQuest, Log,
		TEXT("DebugUnlockChainTo: '%s' 까지 선행 %d건을 수령 완료 처리했습니다."),
		*QuestId.ToString(), Chain.Num() - 1);
}
#endif // !UE_BUILD_SHIPPING

void ULastFPSQuestSubsystem::GrantReward(const FName QuestId, const FLastFPSQuestData& Def)
{
	const int32 PurchaseRefundCredits = ResolvePurchaseRefundCredits(QuestId, Def);
	if (ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		const int64 TotalCredits64 =
			static_cast<int64>(Def.Reward.Credits) + static_cast<int64>(PurchaseRefundCredits);
		const int32 TotalCredits = static_cast<int32>(FMath::Min<int64>(MAX_int32, TotalCredits64));
		if (TotalCredits > 0)
		{
			Economy->AddCredits(TotalCredits);
		}
		for (const FLastFPSItemGrant& Grant : Def.Reward.Items)
		{
			Economy->AddItem(Grant.RowId, Grant.Count);
		}
	}
	const bool bPlayCompletionRadio = PendingCompletionRadioQuestIds.Remove(QuestId) > 0;
	const TArray<FName> CompletionRadioIds = bPlayCompletionRadio
		? Def.RadioOnComplete
		: TArray<FName>();

	// 재화 변경 브로드캐스트까지는 재진입 가드로 보호했다. 팝업 실패 콜백도 즉시 실행될 수 있으므로
	// 표시 단계에 들어가기 전에 가드를 해제해 후속 전이가 유실되지 않게 한다.
	bProcessingTransitions = false;
	NotifyRewardGranted(
		QuestId,
		Def,
		PurchaseRefundCredits,
		FSimpleDelegate::CreateWeakLambda(
			this,
			[this, QuestId, CompletionRadioIds]()
			{
				// 완료 대사를 먼저 넣어 다음 퀘스트 시작 대사가 같은 큐에서 뒤따르게 한다.
				TriggerRadioByIds(CompletionRadioIds);
				ProcessQuestTransitions();
				BroadcastStateChanged();

				// 던전 퀘스트 보상을 모두 닫았다면 허브 맵으로 귀환한다.
				if (IsQuestMappedToAnyMap(QuestId))
				{
					if (UGameInstance* GI = GetGameInstance())
					{
						if (ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(GI->GetFirstLocalPlayerController()))
						{
							PC->ClientReturnToHub();
						}
					}
				}
			}));
}

bool ULastFPSQuestSubsystem::ProcessQuestTransitions()
{
	// 재진입 차단 — GrantReward 의 인벤토리 브로드캐스트가 이 함수를 다시 부르면
	// 여기서 즉시 반환하고, 진행 중인 외부 루프가 이어서 처리한다.
	if (bProcessingTransitions)
	{
		return false;
	}
	bProcessingTransitions = true;

	bool bAny = false;
	bool bLoop = true;
	// 상태 전이는 단조(Completed→Claimed, Locked→NotStarted/InProgress)라 반드시 수렴한다.
	while (bLoop)
	{
		bLoop = false;
		for (TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
		{
			const FLastFPSQuestData* Def = FindQuest(Pair.Key);
			if (!Def)
			{
				continue;
			}

			// 1) 자동 수령 — 완료 + bAutoClaim.
			if (Pair.Value.Status == ELastFPSQuestStatus::Completed && Def->bAutoClaim)
			{
				Pair.Value.Status = ELastFPSQuestStatus::Claimed; // 래치 먼저
				// 수령이 끝난 퀘스트는 더 안내할 게 없다. 자리를 비워 다음 퀘스트가 승계하게 한다.
				ClearTrackedQuestIfMatches(Pair.Key);
				GrantReward(Pair.Key, *Def);
				// 보상 팝업이 닫힌 뒤 콜백이 연쇄 전이를 재개한다. 그 전에 다음 대사를 큐에 넣지 않는다.
				bProcessingTransitions = false;
				return true;
			}

			// 선행 조건 자체가 잠금 해제의 단일 기준이다. NextQuestId가 빠진 데이터도
			// PrereqQuestId 계약대로 복구해, 양방향 체인 필드의 불일치가 진행을 막지 않게 한다.
			if (Pair.Value.Status == ELastFPSQuestStatus::Locked
				&& !Def->PrereqQuestId.IsNone()
				&& GetStatus(Def->PrereqQuestId) == ELastFPSQuestStatus::Claimed)
			{
				Pair.Value.Status = ELastFPSQuestStatus::NotStarted;
				if (Def->QuestGiverNPC.IsNone())
				{
					AcceptQuestInternal(Pair.Key, Pair.Value, *Def);
				}
				bAny = bLoop = true;
			}

			// 2) 다음 퀘스트 해금/수락 — Claimed 이고 NextQuestId 지정.
			if (Pair.Value.Status == ELastFPSQuestStatus::Claimed && !Def->NextQuestId.IsNone())
			{
				if (AdvanceToNext(Def->NextQuestId))
				{
					bAny = bLoop = true;
				}
			}
		}
	}

	bProcessingTransitions = false;
	return bAny;
}

bool ULastFPSQuestSubsystem::AdvanceToNext(FName NextQuestId)
{
	FLastFPSQuestRuntimeState* Next = RuntimeStates.Find(NextQuestId);
	const FLastFPSQuestData* NextDef = FindQuest(NextQuestId);
	if (!Next || !NextDef)
	{
		return false;
	}

	if (NextDef->QuestGiverNPC.IsNone())
	{
		// 스토리 체인형 — 시작 전(Locked/NotStarted)이면 즉시 자동 수락.
		if (Next->Status == ELastFPSQuestStatus::Locked || Next->Status == ELastFPSQuestStatus::NotStarted)
		{
			Next->Status = ELastFPSQuestStatus::NotStarted;
			return AcceptQuestInternal(NextQuestId, *Next, *NextDef);
		}
		return false;
	}

	// 대화 수락형 — Locked 만 해금(NotStarted). 수락은 QuestGiverNPC 대화 시.
	if (Next->Status == ELastFPSQuestStatus::Locked)
	{
		Next->Status = ELastFPSQuestStatus::NotStarted;
		return true;
	}
	return false;
}

void ULastFPSQuestSubsystem::NotifyRewardGranted(
	const FName QuestId,
	const FLastFPSQuestData& Def,
	const int32 PurchaseRefundCredits,
	FSimpleDelegate OnClosed)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		OnClosed.ExecuteIfBound();
		return;
	}
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(GI->GetFirstLocalPlayerController());
	if (!PC)
	{
		OnClosed.ExecuteIfBound();
		return;
	}

	// 던전 퀘스트는 결과 화면으로, 그 외 일반 퀘스트는 기존 공지로 알린다.
	// 어떤 퀘스트가 던전인지는 DungeonMapQuestMap 데이터가 정하므로 여기에 퀘스트 이름을 박지 않는다.
	if (IsQuestMappedToAnyMap(QuestId))
	{
		FLastFPSMissionResult Result;
		Result.MissionName = Def.Title;
		Result.Credits = Def.Reward.Credits;
		Result.PurchaseRefundCredits = PurchaseRefundCredits;
		Result.Items = Def.Reward.Items;
		if (MissionStartRealTimeSeconds >= 0.0)
		{
			Result.ElapsedSeconds =
				static_cast<float>(FPlatformTime::Seconds() - MissionStartRealTimeSeconds);
		}

		PC->ShowMissionResultAfterClosed(Result, MoveTemp(OnClosed));
		return;
	}

	// 보상이 아예 없는 퀘스트(대화로만 끝나는 중간 다리 퀘스트 등)는 '보상 수령' 팝업을 띄우는 것이 어색하므로 팝업을 생략한다.
	const bool bHasAnyReward = (Def.Reward.Credits > 0)
		|| (PurchaseRefundCredits > 0)
		|| (Def.Reward.Items.Num() > 0)
		|| (!Def.RewardText.IsEmpty());

	if (!bHasAnyReward)
	{
		OnClosed.ExecuteIfBound();
		return;
	}

	const FText Title = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardTitle);
	PC->ShowNoticeAfterClosed(Title, BuildRewardMessage(Def, PurchaseRefundCredits), MoveTemp(OnClosed));
}

FText ULastFPSQuestSubsystem::BuildRewardMessage(
	const FLastFPSQuestData& Def,
	const int32 PurchaseRefundCredits) const
{
	// 실제 지급된 구조화 보상(Reward)을 소스로 내역을 만든다 — RewardText 수기 표기와의 드리프트 방지.
	TArray<FText> Lines;
	if (PurchaseRefundCredits > 0)
	{
		Lines.Add(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardPurchaseRefundFormat),
			FText::AsNumber(PurchaseRefundCredits)));
	}
	if (Def.Reward.Credits > 0)
	{
		Lines.Add(FText::Format(
			FLastFPSLocalization::GetUIText(
				Def.Reward.PurchaseRefund.IsEnabled()
					? LastFPSUIStringKeys::QuestRewardCompletionBonusFormat
					: LastFPSUIStringKeys::QuestRewardCreditsFormat),
			FText::AsNumber(Def.Reward.Credits)));
	}

	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	for (const FLastFPSItemGrant& Grant : Def.Reward.Items)
	{
		const FText Name = Economy ? Economy->GetItemDisplayName(Grant.RowId) : FText::FromName(Grant.RowId);
		Lines.Add(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardItemFormat),
			Name,
			FText::AsNumber(Grant.Count)));
	}

	FText RewardBlock = Lines.IsEmpty()
		? FText::GetEmpty()
		: FText::Join(FText::FromString(TEXT("\n")), Lines);
	if (RewardBlock.IsEmpty() && !Def.RewardText.IsEmpty())
	{
		RewardBlock = Def.RewardText;
	}

	if (RewardBlock.IsEmpty())
	{
		return Def.Title;
	}
	return FText::Format(
		FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardMessageFormat),
		Def.Title,
		RewardBlock);
}

int32 ULastFPSQuestSubsystem::ResolvePurchaseRefundCredits(
	const FName QuestId,
	const FLastFPSQuestData& Def) const
{
	const FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	return State
		? Def.Reward.PurchaseRefund.CalculateCredits(State->EligiblePurchaseSpend)
		: 0;
}

bool ULastFPSQuestSubsystem::IsQuestMappedToCurrentMap(const FName QuestId) const
{
	return !QuestId.IsNone() && CurrentMapQuestIds.Contains(QuestId);
}

bool ULastFPSQuestSubsystem::IsQuestMappedToAnyMap(const FName QuestId) const
{
	if (QuestId.IsNone())
	{
		return false;
	}

	return QuestId == DefaultDungeonQuestId
		|| DungeonMapQuestMap.ContainsByPredicate(
			[QuestId](const FLastFPSDungeonQuestMapping& Candidate)
			{
				return Candidate.QuestId == QuestId;
			});
}

bool ULastFPSQuestSubsystem::IsQuestInScopeForCurrentMap(const FName QuestId) const
{
	return IsQuestMappedToCurrentMap(QuestId) || !IsQuestMappedToAnyMap(QuestId);
}

void ULastFPSQuestSubsystem::ResetRepeatableQuest(const FName QuestId)
{
	FLastFPSQuestRuntimeState* State = RuntimeStates.Find(QuestId);
	const FLastFPSQuestData* Def = FindQuest(QuestId);
	if (!State || !Def || !Def->bRepeatable)
	{
		return;
	}

	// 아직 끝나지 않은 임무는 건드리지 않는다. 진행 중 재초기화하면 이미 깬 목표가 되돌아간다.
	if (State->Status != ELastFPSQuestStatus::Completed
		&& State->Status != ELastFPSQuestStatus::Claimed)
	{
		return;
	}

	State->Status = ELastFPSQuestStatus::NotStarted;
	State->Progress.Init(0, Def->Objectives.Num());
	State->Baseline.Init(0, Def->Objectives.Num());
	State->RefundedPurchaseQuantity.Init(0, Def->Objectives.Num());
	State->EligiblePurchaseSpend = 0;

	// 지난 회차의 완료 무전 예약이 남아 있으면 이번 회차 시작 직후에 튀어나온다.
	PendingCompletionRadioQuestIds.Remove(QuestId);

	UE_LOG(LogLastFPSQuest, Log,
		TEXT("[Quest] 반복 임무 '%s' 를 재시작을 위해 초기화했습니다."), *QuestId.ToString());
}

void ULastFPSQuestSubsystem::AcceptDungeonQuestForMap(UWorld& World)
{
	// 같은 월드에서 델리게이트가 여러 번 발사될 수 있어 1회만 처리한다(재입장 시엔 다시 수행).
	if (DungeonQuestAcceptedWorld.Get() == &World)
	{
		return;
	}

	// PIE 접두사를 제거한 정확한 영속 월드 경로만 비교해 이름이 비슷한 테스트 맵의 오작동을 막는다.
	const FString CurrentPackageName =
		UWorld::RemovePIEPrefix(World.GetOutermost()->GetName());
	TArray<FName> MappedQuestIds;
	for (const FLastFPSDungeonQuestMapping& Candidate : DungeonMapQuestMap)
	{
		const FString ConfiguredPackageName =
			Candidate.World.ToSoftObjectPath().GetLongPackageName();
		if (ConfiguredPackageName.IsEmpty() || ConfiguredPackageName != CurrentPackageName)
		{
			continue;
		}

		const FName QuestId = Candidate.QuestId.IsNone() ? DefaultDungeonQuestId : Candidate.QuestId;
		if (!QuestId.IsNone())
		{
			MappedQuestIds.AddUnique(QuestId);
		}
	}

	const bool bScopeChanged = CurrentMapQuestIds != MappedQuestIds;
	CurrentMapQuestIds = MoveTemp(MappedQuestIds);

	if (bScopeChanged)
	{
		BroadcastStateChanged();
	}

	if (CurrentMapQuestIds.IsEmpty())
	{
		return; // 던전 진행 대상 맵이 아니다.
	}

	DungeonQuestAcceptedWorld = &World;

	// 결과 화면의 클리어 시간 기준점. 재입장하면 다시 잰다.
	// 월드 시간은 레벨을 넘나들며 초기화되므로 실시간(FPlatformTime)으로 측정한다.
	MissionStartRealTimeSeconds = FPlatformTime::Seconds();

	for (const FName TargetQuestId : CurrentMapQuestIds)
	{
		// 반복 임무는 재입장할 때마다 새로 시작한다. 상태 전이가 단조라 초기화하지 않으면
		// 두 번째 입장부터 Claimed 로 남아 목표도 브리핑도 나오지 않는다.
		ResetRepeatableQuest(TargetQuestId);

		const ELastFPSQuestStatus Status = GetStatus(TargetQuestId);
		if (Status == ELastFPSQuestStatus::NotStarted)
		{
			UE_LOG(
				LogLastFPSQuest,
				Log,
				TEXT("[Quest] 던전 맵 진입 — 퀘스트 '%s' 자동 수락: %s"),
				*TargetQuestId.ToString(),
				*CurrentPackageName);
			AcceptQuest(TargetQuestId);
			continue;
		}

		// 개발용 시드나 재진입으로 이미 진행 중이어도 해당 전투 레벨의 시작 브리핑은 재생한다.
		if (Status == ELastFPSQuestStatus::InProgress)
		{
			if (const FLastFPSQuestData* Definition = FindQuest(TargetQuestId))
			{
				TriggerRadioByIds(Definition->RadioOnStart);
			}
		}
	}
}

void ULastFPSQuestSubsystem::NotifyObjectiveCompleted(
	const FLastFPSQuestObjective& Objective,
	const int32 PreviousProgress,
	const int32 NewProgress,
	const int32 RequiredCount)
{
	// 부팅 시드나 이미 충족돼 있던 목표는 무전을 다시 재생하지 않는다(경계: 재진입/재계산).
	if (bSuppressObjectiveRadio
		|| Objective.RadioOnComplete.IsEmpty()
		|| PreviousProgress >= RequiredCount
		|| NewProgress < RequiredCount)
	{
		return;
	}

	TriggerRadioByIds(Objective.RadioOnComplete);
}

// ── 이동 동선 (레벨 배치 지점) ──────────────────────────────────────────────

void ULastFPSQuestSubsystem::ScanObjectivePaths(UWorld& World)
{
	// 월드가 바뀌면 좌표도 진행 인덱스도 의미를 잃는다(퀘스트 상태는 GameInstance 수명이라 별도 유지).
	ObjectiveRoutes.Reset();
	RouteCursors.Reset();

	// 순서 태그로 임시 정렬한 뒤 좌표 배열만 남긴다.
	struct FScannedPoint
	{
		int32 Order = 0;
		FVector Location = FVector::ZeroVector;
	};
	TMap<FGameplayTag, TArray<FScannedPoint>> Collected;

	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		const AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		FGameplayTag RouteTag;
		int32 Order = 0;
		if (!ParsePathPointTags(*Actor, RouteTag, Order))
		{
			continue;
		}

		Collected.FindOrAdd(RouteTag).Add(FScannedPoint{ Order, Actor->GetActorLocation() });
	}

	for (TPair<FGameplayTag, TArray<FScannedPoint>>& Pair : Collected)
	{
		Pair.Value.Sort([](const FScannedPoint& A, const FScannedPoint& B) { return A.Order < B.Order; });

		TArray<FVector>& Points = ObjectiveRoutes.Add(Pair.Key);
		Points.Reserve(Pair.Value.Num());
		for (const FScannedPoint& Point : Pair.Value)
		{
			Points.Add(Point.Location);
		}
	}

	UE_LOG(
		LogLastFPSQuest,
		Log,
		TEXT("[Quest] 이동 동선 %d개를 구성했습니다: %s"),
		ObjectiveRoutes.Num(),
		*World.GetName());
}

bool ULastFPSQuestSubsystem::ParsePathPointTags(
	const AActor& Actor,
	FGameplayTag& OutRouteTag,
	int32& OutOrder) const
{
	FGameplayTag RouteTag;
	int32 Order = 0;
	bool bHasOrder = false;

	for (const FName& Tag : Actor.Tags)
	{
		const FString TagString = Tag.ToString();
		if (!PathOrderTagPrefix.IsEmpty() && TagString.StartsWith(PathOrderTagPrefix))
		{
			// 순서 태그가 비정상이어도 0으로 두고 나머지 지점 순서를 살린다(배치 실수 허용).
			Order = FCString::Atoi(*TagString.RightChop(PathOrderTagPrefix.Len()));
			bHasOrder = true;
			continue;
		}

		if (RouteTag.IsValid())
		{
			continue;
		}

		// 등록된 Gameplay Tag 만 경로 식별자로 인정한다 — 일반 액터 태그와 섞이지 않게.
		const FGameplayTag Candidate = FGameplayTag::RequestGameplayTag(Tag, /*ErrorIfNotFound=*/false);
		if (Candidate.IsValid()
			&& (!PathRouteTagRoot.IsValid() || Candidate.MatchesTag(PathRouteTagRoot)))
		{
			RouteTag = Candidate;
		}
	}

	if (!RouteTag.IsValid())
	{
		return false; // 위치 목표와 무관한 액터
	}

	if (!bHasOrder)
	{
		UE_LOG(
			LogLastFPSQuest,
			Warning,
			TEXT("[Quest] 동선 지점 '%s'(%s) 에 순서 태그(%s+숫자)가 없어 무시합니다."),
			*Actor.GetName(),
			*RouteTag.ToString(),
			*PathOrderTagPrefix);
		return false;
	}

	OutRouteTag = RouteTag;
	OutOrder = Order;
	return true;
}

void ULastFPSQuestSubsystem::AdvanceRouteProgress(FGameplayTag RouteTag, const FVector& From)
{
	const TArray<FVector>* Points = ObjectiveRoutes.Find(RouteTag);
	if (!Points || Points->Num() == 0)
	{
		return;
	}

	const int32 LastIndex = Points->Num() - 1;
	int32* ExistingCursor = RouteCursors.Find(RouteTag);
	if (!ExistingCursor)
	{
		// 동선을 처음 시작하는 순간에만 현재 위치에 맞춰 시작 지점을 고른다.
		// (앞 목표를 끝낸 뒤 동선 중간에서 이어받는 경우, 지나온 구간으로 되돌리지 않기 위함)
		int32 NearestIndex = 0;
		double NearestDistSq = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index <= LastIndex; ++Index)
		{
			const double DistSq = FVector::DistSquared(From, (*Points)[Index]);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestIndex = Index;
			}
		}
		ExistingCursor = &RouteCursors.Add(RouteTag, NearestIndex);
	}

	int32& Cursor = *ExistingCursor;
	const double ReachDistSq = FMath::Square(static_cast<double>(FMath::Max(ReachedPointDistance, 0.f)));

	// 한 번 넘긴 지점으로는 돌아가지 않는다(단조 진행).
	while (Cursor < LastIndex)
	{
		const double DistToCurrentSq = FVector::DistSquared(From, (*Points)[Cursor]);
		if (DistToCurrentSq <= ReachDistSq)
		{
			++Cursor; // 지점에 닿음 → 다음 지점 안내
			continue;
		}

		// 빠르게 지나쳐 반경 밖으로 벗어난 경우도 다음 지점이 더 가까우면 진행으로 본다.
		if (FVector::DistSquared(From, (*Points)[Cursor + 1]) < DistToCurrentSq)
		{
			++Cursor;
			continue;
		}
		break;
	}
}

bool ULastFPSQuestSubsystem::GetCurrentRoutePoint(
	FGameplayTag RouteTag,
	FVector& OutLocation,
	bool& bOutIsDestination) const
{
	const TArray<FVector>* Points = ObjectiveRoutes.Find(RouteTag);
	if (!Points || Points->Num() == 0)
	{
		return false;
	}

	const int32 LastIndex = Points->Num() - 1;
	const int32* Cursor = RouteCursors.Find(RouteTag);
	const int32 Index = Cursor ? FMath::Clamp(*Cursor, 0, LastIndex) : LastIndex;

	OutLocation = (*Points)[Index];
	bOutIsDestination = (Index == LastIndex);
	return true;
}

bool ULastFPSQuestSubsystem::GetRouteDestination(FGameplayTag RouteTag, FVector& OutLocation) const
{
	const TArray<FVector>* Points = ObjectiveRoutes.Find(RouteTag);
	if (!Points || Points->Num() == 0)
	{
		return false;
	}

	OutLocation = Points->Last();
	return true;
}
