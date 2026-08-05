#include "Quest/LastFPSQuestSubsystem.h"

#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Game/LastFPSPlayerController.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "Localization/LastFPSLocalization.h"

#include "Engine/DataTable.h"
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
	return GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Quest_Definition) : nullptr;
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

void ULastFPSQuestSubsystem::SeedRuntimeStates()
{
	const UDataTable* Table = GetQuestTable();
	if (!Table)
	{
		UE_LOG(LogLastFPSQuest, Warning, TEXT("GameDataSet에서 DT_QuestData를 찾지 못해 런타임 상태를 만들 수 없습니다."));
		return;
	}

	// 시드 재계산으로 이미 충족된 목표가 완료 무전을 소급 재생하지 않도록 막는다.
	bSuppressObjectiveRadio = true;

	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::SeedRuntimeStates"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[this](const FName& RowName, const FLastFPSQuestData& Row)
		{
			FLastFPSQuestRuntimeState State;
			State.Status = Row.Status; // 초기 시드 상태
			State.Progress.Init(0, Row.Objectives.Num());
			State.Baseline.Init(0, Row.Objectives.Num());

			// 부팅부터 활성(InProgress)인 퀘스트는 지금을 수락 시점으로 보고 기준선/진행을 잡는다.
			if (State.Status == ELastFPSQuestStatus::InProgress)
			{
				CaptureBaseline(Row, State);
				RecomputeProgress(RowName, State, Row);
			}

			RuntimeStates.Add(RowName, MoveTemp(State));
		});

	bSuppressObjectiveRadio = false;

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

	const UDataTable* EncounterDefinitions = GetEncounterTable();
	const bool bCanValidateEncounters =
		EncounterDefinitions
		&& EncounterDefinitions->GetRowStruct() == FLastFPSRoomEncounterData::StaticStruct();
	int32 Broken = 0;
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::ValidateReferences"));
	QuestDefinitions->ForeachRow<FLastFPSQuestData>(Ctx,
		[Economy, bCanValidateItems, EncounterDefinitions, bCanValidateEncounters, &Broken](
			const FName& RowName,
			const FLastFPSQuestData& Row)
		{
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

bool ULastFPSQuestSubsystem::CheckCompletion(FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def) const
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

	bChanged |= CheckCompletion(State, Def);
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
			bAnyChanged |= ApplyEventToQuest(Pair.Value, *Def, Event);
		}
	}
	return bAnyChanged;
}

bool ULastFPSQuestSubsystem::ApplyEventToQuest(FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def, const FLastFPSObjectiveEvent& Event)
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

	bChanged |= CheckCompletion(State, Def);
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
	if (OnRadioTransmission.IsBound())
	{
		OnRadioTransmission.Broadcast(RadioData);
		return;
	}

	// 월드 초기화가 로컬 HUD 생성보다 먼저 끝나는 경우에도 입장 무전을 잃지 않는다.
	PendingRadioTransmissions.Add(RadioData);
}

void ULastFPSQuestSubsystem::TriggerRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray)
{
	for (const FLastFPSRadioTransmissionData& RadioData : RadioDataArray)
	{
		TriggerRadioTransmission(RadioData);
	}
}

void ULastFPSQuestSubsystem::TriggerRadioByIds(const TArray<FName>& RadioIds)
{
	for (const FName& RadioId : RadioIds)
	{
		if (const FLastFPSRadioTransmissionData* Data = FindRadioTransmission(RadioId))
		{
			TriggerRadioTransmission(*Data);
		}
		else
		{
			UE_LOG(LogLastFPSQuest, Warning, TEXT("RadioTransmission 행 '%s' 을 찾을 수 없습니다."), *RadioId.ToString());
		}
	}
}

void ULastFPSQuestSubsystem::FlushPendingRadioTransmissions()
{
	if (!OnRadioTransmission.IsBound() || PendingRadioTransmissions.IsEmpty())
	{
		return;
	}

	TArray<FLastFPSRadioTransmissionData> Pending = MoveTemp(PendingRadioTransmissions);
	PendingRadioTransmissions.Reset();

	for (const FLastFPSRadioTransmissionData& RadioData : Pending)
	{
		OnRadioTransmission.Broadcast(RadioData);
	}
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

	bool bChanged = false;

	// 1) 이 NPC 가 수락 트리거(QuestGiverNPC)인 NotStarted 퀘스트를 수락.
	for (TPair<FName, FLastFPSQuestRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.Status != ELastFPSQuestStatus::NotStarted)
		{
			continue;
		}
		const FLastFPSQuestData* Def = FindQuest(Pair.Key);
		if (Def && !Def->QuestGiverNPC.IsNone() && Def->QuestGiverNPC == NPCRowName)
		{
			bChanged |= AcceptQuestInternal(Pair.Key, Pair.Value, *Def);
		}
	}

	// 2) TalkToNPC 목표 진행.
	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::TalkToNPC;
	Event.Id = NPCRowName;
	bChanged |= ApplyObjectiveEventToActive(Event);

	bChanged |= ProcessQuestTransitions();
	if (bChanged)
	{
		BroadcastStateChanged();
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
	const TWeakObjectPtr<USceneComponent>* Found = LocationMarkers.Find(LocationTag);
	if (Found && Found->Get() == Marker)
	{
		LocationMarkers.Remove(LocationTag);
	}
}

bool ULastFPSQuestSubsystem::GetTrackedLocation(FGameplayTag LocationTag, FVector& OutLocation) const
{
	if (const TWeakObjectPtr<USceneComponent>* Found = LocationMarkers.Find(LocationTag))
	{
		if (const USceneComponent* Comp = Found->Get())
		{
			OutLocation = Comp->GetComponentLocation();
			return true;
		}
	}
	return false;
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
	const TWeakObjectPtr<USceneComponent>* Found =
		EncounterMarkers.Find(EncounterId);
	if (Found && Found->Get() == Marker)
	{
		EncounterMarkers.Remove(EncounterId);
	}
}

bool ULastFPSQuestSubsystem::GetTrackedEncounterLocation(
	const FName EncounterId,
	FVector& OutLocation) const
{
	if (const TWeakObjectPtr<USceneComponent>* Found =
		EncounterMarkers.Find(EncounterId))
	{
		if (const USceneComponent* Marker = Found->Get())
		{
			OutLocation = Marker->GetComponentLocation();
			return true;
		}
	}

	return false;
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

	// 안내 지점의 전진은 위치 폴이 담당하고, 여기서는 현재 지점을 읽기만 한다.
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
			if (Def->bSequentialObjectives && Progress >= RequiredCount)
			{
				continue;
			}

			const bool bIsReachLocation =
				Obj.Type == ELastFPSObjectiveType::ReachLocation;
			const bool bIsClearEncounter =
				Obj.Type == ELastFPSObjectiveType::ClearEncounter;
			if (!bIsReachLocation && !bIsClearEncounter)
			{
				if (Def->bSequentialObjectives)
				{
					break;
				}
				continue;
			}
			// 이미 완료된 목표는 마커를 띄우지 않는다.
			if (Progress >= RequiredCount)
			{
				continue;
			}

			FVector MarkerLocation = FVector::ZeroVector;
			bool bIsDestination = true;
			if (!ResolveObjectiveGuidanceLocation(Obj, MarkerLocation, bIsDestination))
			{
				continue;
			}

			FLastFPSObjectiveWaypoint Waypoint;
			Waypoint.WorldLocation = MarkerLocation;
			Waypoint.Label = Obj.Label.IsEmpty() ? Def->Title : Obj.Label;
			Waypoint.QuestId = Pair.Key;
			Waypoint.LocationTag = Obj.TargetTag;
			Waypoint.bIsRoutePoint = !bIsDestination;
			OutWaypoints.Add(Waypoint);
			if (Def->bSequentialObjectives)
			{
				break;
			}
		}
	}
}

bool ULastFPSQuestSubsystem::ResolveObjectiveGuidanceLocation(
	const FLastFPSQuestObjective& Objective,
	FVector& OutLocation,
	bool& bOutIsDestination) const
{
	bOutIsDestination = true;

	if (Objective.Type == ELastFPSObjectiveType::ClearEncounter)
	{
		return GetTrackedEncounterLocation(Objective.TargetId, OutLocation);
	}

	if (Objective.Type != ELastFPSObjectiveType::ReachLocation)
	{
		return false;
	}

	// 동선이 배치돼 있으면 지금 안내 중인 지점, 없으면 도달 지점을 가리킨다.
	if (GetCurrentRoutePoint(Objective.TargetTag, OutLocation, bOutIsDestination))
	{
		return true;
	}

	bOutIsDestination = true;
	return ResolveObjectiveLocation(Objective, OutLocation);
}

void ULastFPSQuestSubsystem::GetTrackedQuests(TArray<FLastFPSTrackedQuest>& OutQuests) const
{
	OutQuests.Reset();

	const UDataTable* Table = GetQuestTable();
	if (!Table)
	{
		return;
	}

	// 표시 순서가 갱신마다 흔들리지 않도록 런타임 상태 맵이 아니라 테이블 행 순서로 훑는다.
	Table->ForeachRow<FLastFPSQuestData>(TEXT("ULastFPSQuestSubsystem::GetTrackedQuests"),
		[this, &OutQuests](const FName& RowName, const FLastFPSQuestData& Row)
		{
			if (GetStatus(RowName) != ELastFPSQuestStatus::InProgress)
			{
				return;
			}

			FLastFPSTrackedQuest& Tracked = OutQuests.AddDefaulted_GetRef();
			Tracked.QuestId = RowName;
			Tracked.Title = Row.Title;
			Tracked.Type = Row.Type;
			Tracked.Category = Row.Category;
			Tracked.Objectives.Reserve(Row.Objectives.Num());

			for (int32 ObjectiveIndex = 0; ObjectiveIndex < Row.Objectives.Num(); ++ObjectiveIndex)
			{
				const FLastFPSQuestObjective& Objective = Row.Objectives[ObjectiveIndex];

				FLastFPSTrackedObjective& Snapshot = Tracked.Objectives.AddDefaulted_GetRef();
				Snapshot.Label = Objective.Label;
				Snapshot.Type = Objective.Type;
				Snapshot.Progress = GetObjectiveProgress(RowName, ObjectiveIndex);
				Snapshot.RequiredCount = ResolveObjectiveRequiredCount(Objective);
				Snapshot.bCompleted = Snapshot.Progress >= Snapshot.RequiredCount;

				bool bIsDestination = true;
				Snapshot.bHasGuidanceLocation = !Snapshot.bCompleted
					&& ResolveObjectiveGuidanceLocation(Objective, Snapshot.GuidanceLocation, bIsDestination);

				// 순차 퀘스트는 현재 단계까지만 안내하고 다음 단계는 미리 공개하지 않는다.
				if (Row.bSequentialObjectives && !Snapshot.bCompleted)
				{
					break;
				}
			}
		});
}

void ULastFPSQuestSubsystem::BroadcastStateChanged()
{
	UpdateLocationPollTimer();
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

bool ULastFPSQuestSubsystem::AcceptQuestInternal(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def)
{
	State.Status = ELastFPSQuestStatus::InProgress;
	State.Progress.Init(0, Def.Objectives.Num());
	CaptureBaseline(Def, State);
	RecomputeProgress(QuestId, State, Def); // 수락 즉시 충족되는 경우(목표 0개 등) 반영

	if (!Def.RadioOnStart.IsEmpty())
	{
		TriggerRadioByIds(Def.RadioOnStart);
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
	GrantReward(*Def);
	ProcessQuestTransitions(); // 다음 퀘스트 해금/자동수락 연쇄
	BroadcastStateChanged();
	return true;
}

void ULastFPSQuestSubsystem::GrantReward(const FLastFPSQuestData& Def)
{
	if (ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		if (Def.Reward.Credits > 0)
		{
			Economy->AddCredits(Def.Reward.Credits);
		}
		for (const FLastFPSItemGrant& Grant : Def.Reward.Items)
		{
			Economy->AddItem(Grant.RowId, Grant.Count);
		}
	}
	NotifyRewardGranted(Def);
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
				GrantReward(*Def);
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

void ULastFPSQuestSubsystem::NotifyRewardGranted(const FLastFPSQuestData& Def) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	if (ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(GI->GetFirstLocalPlayerController()))
	{
		const FText Title = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardTitle);
		PC->ShowNotice(Title, BuildRewardMessage(Def));
	}
}

FText ULastFPSQuestSubsystem::BuildRewardMessage(const FLastFPSQuestData& Def) const
{
	// 실제 지급된 구조화 보상(Reward)을 소스로 내역을 만든다 — RewardText 수기 표기와의 드리프트 방지.
	TArray<FText> Lines;
	if (Def.Reward.Credits > 0)
	{
		Lines.Add(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestRewardCreditsFormat),
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
	const FLastFPSDungeonQuestMapping* Mapping = DungeonMapQuestMap.FindByPredicate(
		[&CurrentPackageName](const FLastFPSDungeonQuestMapping& Candidate)
		{
			const FString ConfiguredPackageName =
				Candidate.World.ToSoftObjectPath().GetLongPackageName();
			return !ConfiguredPackageName.IsEmpty()
				&& ConfiguredPackageName == CurrentPackageName;
		});

	if (!Mapping)
	{
		return; // 던전 진행 대상 맵이 아니다.
	}

	DungeonQuestAcceptedWorld = &World;

	const FName TargetQuestId = Mapping->QuestId.IsNone() ? DefaultDungeonQuestId : Mapping->QuestId;
	if (TargetQuestId.IsNone())
	{
		return;
	}

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
		return;
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
