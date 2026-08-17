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
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	ULastFPSQuestSubsystem* QuestSubsystem = GI ? GI->GetSubsystem<ULastFPSQuestSubsystem>() : nullptr;
	if (!QuestSubsystem)
	{
		return;
	}

	// 도달 판정은 각 머신의 로컬 플레이어 기준이다. 원격 파티원의 시뮬레이션 폰까지 받아주면
	// 내가 가지도 않은 지점이 내 목표에서 달성 처리된다.
	// (ULastFPSObjectiveTriggerComponent 와 동일한 판정)
	if (Pawn->IsLocallyControlled()
		&& LocationTag.IsValid()
		&& !(bTriggerOnce && bAlreadyTriggered))
	{
		bAlreadyTriggered = true;
		QuestSubsystem->NotifyLocationTriggerChanged(LocationTag, true);
	}

	// 무전은 파티 공용이다. 서버가 한 번만 송출하면 지금 접속한 파티원과 이후 합류자 모두 듣는다.
	// 서버에는 모든 파티원의 폰이 있으므로, 누가 밟았는지와 무관하게 여기서 판정한다.
	if (HasAuthority()
		&& !RadioTransmissionIds.IsEmpty()
		&& !(bTriggerOnce && bRadioBroadcast))
	{
		bRadioBroadcast = true;
		QuestSubsystem->Auth_BroadcastPartyRadio(RadioTransmissionIds);
	}
}
