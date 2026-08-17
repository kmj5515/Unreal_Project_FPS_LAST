#include "Game/LastFPSGameStateBase.h"

#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "Game/Loading/LastFPSDestinationContentComponent.h"

#include "Net/UnrealNetwork.h"

ALastFPSGameStateBase::ALastFPSGameStateBase()
{
    DestinationContent =
        CreateDefaultSubobject<ULastFPSDestinationContentComponent>(
            TEXT("DestinationContent"));
}

void ALastFPSGameStateBase::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSGameStateBase, DestinationContextTags);
    DOREPLIFETIME(ALastFPSGameStateBase, DestinationContentSetId);
    DOREPLIFETIME(ALastFPSGameStateBase, SharedQuestProgress);
    DOREPLIFETIME(ALastFPSGameStateBase, SharedRadio);
    DOREPLIFETIME(ALastFPSGameStateBase, MapUIRules);
}

void ALastFPSGameStateBase::Auth_SetMapUIRules(const FLastFPSMapUIRules& NewRules)
{
    if (!HasAuthority())
    {
        return;
    }

    MapUIRules = NewRules;
    MapUIRules.bInitialized = true;

    // 리슨 서버 호스트는 OnRep 을 받지 못하므로 여기서 직접 알린다.
    OnMapUIRulesChanged.Broadcast();
}

void ALastFPSGameStateBase::OnRep_MapUIRules()
{
    OnMapUIRulesChanged.Broadcast();
}

bool ALastFPSGameStateBase::Auth_ApplySharedQuestProgress(
    const FName QuestId,
    const int32 ObjectiveIndex,
    const int32 ObjectiveNum,
    const int32 Value,
    const bool bAbsolute,
    const int32 RequiredCount)
{
    if (!HasAuthority()
        || QuestId.IsNone()
        || ObjectiveNum <= 0
        || !FMath::IsWithin(ObjectiveIndex, 0, ObjectiveNum))
    {
        return false;
    }

    FLastFPSSharedQuestProgress* Entry = SharedQuestProgress.FindByPredicate(
        [QuestId](const FLastFPSSharedQuestProgress& Candidate)
        {
            return Candidate.QuestId == QuestId;
        });
    if (!Entry)
    {
        Entry = &SharedQuestProgress.AddDefaulted_GetRef();
        Entry->QuestId = QuestId;
    }
    // 정의가 바뀌어도 인덱스 대응이 깨지지 않도록 목표 수에 맞춰 둔다.
    Entry->Progress.SetNum(ObjectiveNum);

    const int32 PreviousProgress = Entry->Progress[ObjectiveIndex];
    const int32 NewProgress = FLastFPSSharedQuestProgress::MergeProgress(
        PreviousProgress,
        Value,
        bAbsolute,
        RequiredCount);
    if (NewProgress == PreviousProgress)
    {
        return false;
    }

    // 여기서 브로드캐스트하지 않는다 — 권위 머신은 목표 루프를 다 돈 뒤 한 번만 반영해야 한다.
    // (반영 과정에서 다음 퀘스트가 수락되며 런타임 상태 맵이 바뀔 수 있어 순회 중 재진입은 위험하다.)
    Entry->Progress[ObjectiveIndex] = NewProgress;
    ForceNetUpdate();
    return true;
}

const TArray<int32>* ALastFPSGameStateBase::FindSharedQuestProgress(
    const FName QuestId) const
{
    const FLastFPSSharedQuestProgress* Entry = SharedQuestProgress.FindByPredicate(
        [QuestId](const FLastFPSSharedQuestProgress& Candidate)
        {
            return Candidate.QuestId == QuestId;
        });
    return Entry ? &Entry->Progress : nullptr;
}

void ALastFPSGameStateBase::OnRep_SharedQuestProgress()
{
    OnSharedQuestProgressChanged.Broadcast();
}

void ALastFPSGameStateBase::Auth_BroadcastSharedRadio(const TArray<FName>& RadioIds)
{
    if (!HasAuthority() || RadioIds.IsEmpty())
    {
        return;
    }

    SharedRadio.RadioIds = RadioIds;
    ++SharedRadio.Serial;
    ForceNetUpdate();
}

void ALastFPSGameStateBase::OnRep_SharedRadio()
{
    OnSharedRadioChanged.Broadcast();
}

void ALastFPSGameStateBase::SetDestinationContextTags(
    const FGameplayTagContainer& NewContextTags)
{
    if (!HasAuthority() || DestinationContextTags == NewContextTags)
    {
        return;
    }

    DestinationContextTags = NewContextTags;
    OnDestinationContextChanged.Broadcast(DestinationContextTags);
    ForceNetUpdate();
}

void ALastFPSGameStateBase::SetDestinationContentSet(
    const ULastFPSDestinationContentSet* NewContentSet)
{
    if (!HasAuthority())
    {
        return;
    }

    const FPrimaryAssetId NewContentSetId = NewContentSet
        ? NewContentSet->GetPrimaryAssetId()
        : FPrimaryAssetId();
    if (DestinationContentSetId == NewContentSetId)
    {
        return;
    }

    DestinationContentSetId = NewContentSetId;
    ForceNetUpdate();
}

void ALastFPSGameStateBase::OnRep_DestinationContextTags()
{
    OnDestinationContextChanged.Broadcast(DestinationContextTags);
}

void ALastFPSGameStateBase::OnRep_DestinationContentSetId()
{
    if (DestinationContent && DestinationContentSetId.IsValid())
    {
        DestinationContent->StartContentLoad(DestinationContentSetId);
    }
}
