#include "Quest/LastFPSQuestSubsystem.h"

#include "Economy/LastFPSEconomySubsystem.h"
#include "Game/LastFPSPlayerController.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/SceneComponent.h"
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
		virtual bool RecomputeProgress(const FLastFPSQuestObjective& Obj, int32 Baseline, const FLastFPSObjectiveEvalContext& Ctx, int32& OutProgress) const override
		{
			const int32 Current = Ctx.Economy ? Ctx.Economy->GetItemCount(Obj.TargetId) : 0;
			OutProgress = Current - Baseline; // 클램프는 호출부가 일괄 처리
			return true;
		}
	};

	/** 위치 도달 — 로컬 폰이 마커의 AcceptRadius(m) 이내면 완료(pull형, 이진). */
	class FReachLocationTracker : public ILastFPSObjectiveTracker
	{
	public:
		virtual bool RecomputeProgress(const FLastFPSQuestObjective& Obj, int32 Baseline, const FLastFPSObjectiveEvalContext& Ctx, int32& OutProgress) const override
		{
			OutProgress = 0;
			FVector Target;
			if (Ctx.bHasPlayerLocation && Ctx.Subsystem && Ctx.Subsystem->GetTrackedLocation(Obj.TargetTag, Target))
			{
				const float RadiusCm = Obj.AcceptRadius * 100.f; // m → cm
				if (FVector::DistSquared(Ctx.PlayerLocation, Target) <= FMath::Square(RadiusCm))
				{
					OutProgress = Obj.RequiredCount;
				}
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
}

void ULastFPSQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Economy 를 먼저 초기화(진행 판정/보상 지급이 Economy 를 사용). 시드 아이템도 이 시점엔 이미 채워져 있음.
	Collection.InitializeDependency<ULastFPSEconomySubsystem>();

	Super::Initialize(Collection);

	BuildTrackers();
	SeedRuntimeStates();
	ValidateReferences();
	UpdateLocationPollTimer();

	// 보유 변동 구독 — AcquireItem 진행 자동 추적. (Economy 시드는 델리게이트를 안 태우므로 위 SeedRuntimeStates 에서 기준선/초기진행을 이미 반영.)
	if (ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		Economy->OnInventoryChanged.AddDynamic(this, &ULastFPSQuestSubsystem::HandleInventoryChanged);
		bInventorySubscribed = true;
	}
}

void ULastFPSQuestSubsystem::Deinitialize()
{
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

void ULastFPSQuestSubsystem::BuildTrackers()
{
	Trackers.Add(ELastFPSObjectiveType::AcquireItem, MakeUnique<FAcquireItemTracker>());
	Trackers.Add(ELastFPSObjectiveType::ReachLocation, MakeUnique<FReachLocationTracker>());
	Trackers.Add(ELastFPSObjectiveType::KillTarget, MakeUnique<FKillTargetTracker>());
	Trackers.Add(ELastFPSObjectiveType::TalkToNPC, MakeUnique<FTalkToNPCTracker>());
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
	return QuestTable.LoadSynchronous();
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
		UE_LOG(LogLastFPSQuest, Warning,
			TEXT("QuestTable(DT_QuestData) 미설정 — DefaultGame.ini 의 [/Script/LastFPS.LastFPSQuestSubsystem] QuestTable 확인."));
		return;
	}

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
}

void ULastFPSQuestSubsystem::ValidateReferences() const
{
#if !UE_BUILD_SHIPPING
	const UDataTable* Table = GetQuestTable();
	if (!Table)
	{
		return; // SeedRuntimeStates 에서 이미 경고
	}

	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	if (!Economy || !Economy->IsItemTableConfigured())
	{
		// ItemTable 미설정 시 HasItemDefinition 이 전부 false → 오탐이 되므로 검증을 건너뛴다.
		return;
	}

	int32 Broken = 0;
	static const FString Ctx(TEXT("ULastFPSQuestSubsystem::ValidateReferences"));
	Table->ForeachRow<FLastFPSQuestData>(Ctx,
		[Economy, &Broken](const FName& RowName, const FLastFPSQuestData& Row)
		{
			for (const FLastFPSQuestObjective& Obj : Row.Objectives)
			{
				if (Obj.Type == ELastFPSObjectiveType::AcquireItem
					&& !Obj.TargetId.IsNone()
					&& !Economy->HasItemDefinition(Obj.TargetId))
				{
					++Broken;
					UE_LOG(LogLastFPSQuest, Error,
						TEXT("[Quest] '%s' 의 목표 아이템 '%s' 가 DT_ItemData 에 없음 — 영원히 진행 불가."),
						*RowName.ToString(), *Obj.TargetId.ToString());
				}
			}

			for (const FLastFPSItemGrant& Grant : Row.Reward.Items)
			{
				if (!Grant.RowId.IsNone() && !Economy->HasItemDefinition(Grant.RowId))
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
		if (Prog < Def.Objectives[i].RequiredCount)
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
			NewProgress = FMath::Clamp(NewProgress, 0, Obj.RequiredCount);
			if (State.Progress[i] != NewProgress)
			{
				State.Progress[i] = NewProgress;
				bChanged = true;
			}
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
	if (RecomputeAllActive())
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::HandleLocationPoll()
{
	if (RecomputeAllActive())
	{
		BroadcastStateChanged();
	}
}

void ULastFPSQuestSubsystem::ApplyObjectiveEvent(const FLastFPSObjectiveEvent& Event)
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

	if (bAnyChanged)
	{
		BroadcastStateChanged();
	}
}

bool ULastFPSQuestSubsystem::ApplyEventToQuest(FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def, const FLastFPSObjectiveEvent& Event)
{
	State.Progress.SetNum(Def.Objectives.Num());

	bool bChanged = false;
	for (int32 i = 0; i < Def.Objectives.Num(); ++i)
	{
		const FLastFPSQuestObjective& Obj = Def.Objectives[i];
		const ILastFPSObjectiveTracker* Tracker = GetTracker(Obj.Type);
		if (Tracker && Tracker->MatchesEvent(Event, Obj))
		{
			const int32 NewProgress = FMath::Clamp(State.Progress[i] + Event.Count, 0, Obj.RequiredCount);
			if (NewProgress != State.Progress[i])
			{
				State.Progress[i] = NewProgress;
				bChanged = true;
			}
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
	ApplyObjectiveEvent(Event);
}

void ULastFPSQuestSubsystem::NotifyTalkedToNPC(FName NPCRowName)
{
	if (NPCRowName.IsNone())
	{
		return;
	}
	FLastFPSObjectiveEvent Event;
	Event.Type = ELastFPSObjectiveType::TalkToNPC;
	Event.Id = NPCRowName;
	ApplyObjectiveEvent(Event);
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

void ULastFPSQuestSubsystem::GetActiveWaypoints(TArray<FLastFPSObjectiveWaypoint>& OutWaypoints) const
{
	OutWaypoints.Reset();

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
			if (Obj.Type != ELastFPSObjectiveType::ReachLocation)
			{
				continue;
			}
			// 이미 도달한 목표는 마커를 띄우지 않는다.
			const int32 Prog = Pair.Value.Progress.IsValidIndex(i) ? Pair.Value.Progress[i] : 0;
			if (Prog >= Obj.RequiredCount)
			{
				continue;
			}

			FVector Location;
			if (!GetTrackedLocation(Obj.TargetTag, Location))
			{
				continue; // 레벨에 마커가 아직 없으면 건너뜀
			}

			FLastFPSObjectiveWaypoint Waypoint;
			Waypoint.WorldLocation = Location;
			Waypoint.Label = Obj.Label.IsEmpty() ? Def->Title : Obj.Label;
			Waypoint.QuestId = Pair.Key;
			OutWaypoints.Add(Waypoint);
		}
	}
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
		for (const FLastFPSQuestObjective& Obj : Def->Objectives)
		{
			if (Obj.Type == ELastFPSObjectiveType::ReachLocation)
			{
				bNeedPoll = true;
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
	if (!State || !Def || State->Status != ELastFPSQuestStatus::NotStarted)
	{
		return false;
	}

	State->Status = ELastFPSQuestStatus::InProgress;
	State->Progress.Init(0, Def->Objectives.Num());
	CaptureBaseline(*Def, *State);
	RecomputeProgress(QuestId, *State, *Def); // 수락 즉시 충족되는 경우(목표 0개 등) 반영

	BroadcastStateChanged();
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

	if (ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		if (Def->Reward.Credits > 0)
		{
			Economy->AddCredits(Def->Reward.Credits);
		}
		for (const FLastFPSItemGrant& Grant : Def->Reward.Items)
		{
			Economy->AddItem(Grant.RowId, Grant.Count);
		}
	}

	NotifyRewardGranted(*Def);
	BroadcastStateChanged();
	return true;
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
		const FText Title = NSLOCTEXT("LastFPS", "Quest_RewardTitle", "보상 수령");
		PC->ShowNotice(Title, BuildRewardMessage(Def));
	}
}

FText ULastFPSQuestSubsystem::BuildRewardMessage(const FLastFPSQuestData& Def) const
{
	// 실제 지급된 구조화 보상(Reward)을 소스로 내역을 만든다 — RewardText 수기 표기와의 드리프트 방지.
	TArray<FString> Lines;
	if (Def.Reward.Credits > 0)
	{
		Lines.Add(FString::Printf(TEXT("크레딧 +%d"), Def.Reward.Credits));
	}

	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	for (const FLastFPSItemGrant& Grant : Def.Reward.Items)
	{
		const FText Name = Economy ? Economy->GetItemDisplayName(Grant.RowId) : FText::FromName(Grant.RowId);
		Lines.Add(FString::Printf(TEXT("%s ×%d"), *Name.ToString(), Grant.Count));
	}

	FString RewardBlock = FString::Join(Lines, TEXT("\n"));
	if (RewardBlock.IsEmpty() && !Def.RewardText.IsEmpty())
	{
		RewardBlock = Def.RewardText.ToString(); // 구조화 보상 없음 → 수기 표기 폴백
	}

	if (RewardBlock.IsEmpty())
	{
		return Def.Title; // 보상 정보 자체가 없으면 기존 동작(제목만)
	}
	return FText::FromString(FString::Printf(TEXT("%s\n\n%s"), *Def.Title.ToString(), *RewardBlock));
}
