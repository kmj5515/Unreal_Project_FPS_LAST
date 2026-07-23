#include "Quest/LastFPSObjectiveTriggerComponent.h"

#include "Quest/LastFPSQuestSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSObjectiveTrigger, Log, All);

ULastFPSObjectiveTriggerComponent::ULastFPSObjectiveTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 폰만 겹치는 쿼리 전용 볼륨 — 물리 충돌/차단은 만들지 않는다.
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldStatic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

void ULastFPSObjectiveTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!LocationTag.IsValid())
	{
		UE_LOG(LogLastFPSObjectiveTrigger, Warning,
			TEXT("[Quest] ObjectiveTrigger(%s) LocationTag 미지정 — 등록하지 않음."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
		return;
	}

	if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
	{
		// HUD 화면 마커가 이 박스를 가리키도록 위치도 등록(반경 마커와 동일 등록소).
		Quest->RegisterLocationMarker(LocationTag, this);
		bRegistered = true;
	}

	OnComponentBeginOverlap.AddDynamic(this, &ULastFPSObjectiveTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &ULastFPSObjectiveTriggerComponent::HandleEndOverlap);
}

void ULastFPSObjectiveTriggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnComponentBeginOverlap.RemoveDynamic(this, &ULastFPSObjectiveTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.RemoveDynamic(this, &ULastFPSObjectiveTriggerComponent::HandleEndOverlap);

	if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
	{
		// 안에 있는 채로 파괴되면 EndOverlap 이 안 올 수 있어 겹침 카운트를 명시적으로 되돌린다.
		if (bPlayerInside)
		{
			Quest->NotifyLocationTriggerChanged(LocationTag, false);
		}
		if (bRegistered)
		{
			Quest->UnregisterLocationMarker(LocationTag, this);
		}
	}
	bPlayerInside = false;
	bRegistered = false;

	Super::EndPlay(EndPlayReason);
}

void ULastFPSObjectiveTriggerComponent::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	// 폰의 여러 컴포넌트가 겹쳐도 진입은 1회만 통지.
	if (bPlayerInside || !IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	bPlayerInside = true;

	if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
	{
		Quest->NotifyLocationTriggerChanged(LocationTag, true);
	}
}

void ULastFPSObjectiveTriggerComponent::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!bPlayerInside || !IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	// 폰의 다른 컴포넌트가 아직 겹쳐 있으면 실제로 나간 게 아니다.
	if (IsOverlappingActor(OtherActor))
	{
		return;
	}
	bPlayerInside = false;

	if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
	{
		Quest->NotifyLocationTriggerChanged(LocationTag, false);
	}
}

bool ULastFPSObjectiveTriggerComponent::IsLocalPlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
}

ULastFPSQuestSubsystem* ULastFPSObjectiveTriggerComponent::GetQuestSubsystem() const
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
