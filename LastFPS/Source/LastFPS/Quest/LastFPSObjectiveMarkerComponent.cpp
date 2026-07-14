#include "Quest/LastFPSObjectiveMarkerComponent.h"

#include "Quest/LastFPSQuestSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSObjectiveMarker, Log, All);

ULastFPSObjectiveMarkerComponent::ULastFPSObjectiveMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULastFPSObjectiveMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!LocationTag.IsValid())
	{
		UE_LOG(LogLastFPSObjectiveMarker, Warning,
			TEXT("[Quest] ObjectiveMarker(%s) LocationTag 미지정 — 등록하지 않음."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
		return;
	}

	if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
	{
		Quest->RegisterLocationMarker(LocationTag, this);
		bRegistered = true;
	}
}

void ULastFPSObjectiveMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRegistered)
	{
		if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
		{
			Quest->UnregisterLocationMarker(LocationTag, this);
		}
		bRegistered = false;
	}

	Super::EndPlay(EndPlayReason);
}

ULastFPSQuestSubsystem* ULastFPSObjectiveMarkerComponent::GetQuestSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<ULastFPSQuestSubsystem>();
		}
	}
	return nullptr;
}
