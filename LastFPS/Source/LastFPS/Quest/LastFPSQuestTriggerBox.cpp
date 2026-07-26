#include "Quest/LastFPSQuestTriggerBox.h"

#include "GameFramework/Pawn.h"
#include "Quest/LastFPSQuestSubsystem.h"

ALastFPSQuestTriggerBox::ALastFPSQuestTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALastFPSQuestTriggerBox::ConfigureQuestTrigger(const FGameplayTag& InLocationTag, const TArray<FName>& InRadioIds)
{
	LocationTag = InLocationTag;
	RadioTransmissionIds = InRadioIds;
}

void ALastFPSQuestTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &ALastFPSQuestTriggerBox::OnOverlapBegin);
}

void ALastFPSQuestTriggerBox::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (bTriggerOnce && bAlreadyTriggered)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	bAlreadyTriggered = true;

	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (ULastFPSQuestSubsystem* QuestSubsystem = GI->GetSubsystem<ULastFPSQuestSubsystem>())
			{
				if (LocationTag.IsValid())
				{
					QuestSubsystem->NotifyLocationTriggerChanged(LocationTag, true);
				}

				if (!RadioTransmissionIds.IsEmpty())
				{
					QuestSubsystem->TriggerRadioByIds(RadioTransmissionIds);
				}
			}
		}
	}
}
