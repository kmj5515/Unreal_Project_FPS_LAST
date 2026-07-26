#include "Defend/LastFPSCaptureZoneComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSCapture, Log, All);

ULastFPSCaptureZoneComponent::ULastFPSCaptureZoneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 폰만 겹치는 쿼리 전용 볼륨 — 물리 차단은 만들지 않는다(도달 트리거와 동일 규칙).
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldStatic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

void ULastFPSCaptureZoneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULastFPSCaptureZoneComponent, ElapsedSeconds);
}

void ULastFPSCaptureZoneComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &ULastFPSCaptureZoneComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &ULastFPSCaptureZoneComponent::HandleEndOverlap);
}

void ULastFPSCaptureZoneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureTimerHandle);
	}
	OnComponentBeginOverlap.RemoveDynamic(this, &ULastFPSCaptureZoneComponent::HandleBeginOverlap);
	OnComponentEndOverlap.RemoveDynamic(this, &ULastFPSCaptureZoneComponent::HandleEndOverlap);

	Super::EndPlay(EndPlayReason);
}

bool ULastFPSCaptureZoneComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

bool ULastFPSCaptureZoneComponent::IsLocalPlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsLocallyControlled();
}

ULastFPSQuestSubsystem* ULastFPSCaptureZoneComponent::GetQuestSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>() : nullptr;
}

void ULastFPSCaptureZoneComponent::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!HasAuthority() || bCaptured || !IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	SetPlayerInside(true);
}

void ULastFPSCaptureZoneComponent::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (!HasAuthority() || !IsLocalPlayerPawn(OtherActor))
	{
		return;
	}
	SetPlayerInside(false);
}

void ULastFPSCaptureZoneComponent::SetPlayerInside(bool bInside)
{
	if (bPlayerInside == bInside)
	{
		return;
	}
	bPlayerInside = bInside;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 안에 있는 동안만 타이머를 돌리고, 나가면 멈춘다(진행은 유지, 감소 없음).
	if (bPlayerInside && !bCaptured)
	{
		World->GetTimerManager().SetTimer(
			CaptureTimerHandle, this, &ULastFPSCaptureZoneComponent::TickCapture, UpdateInterval, true);
	}
	else
	{
		World->GetTimerManager().ClearTimer(CaptureTimerHandle);
	}
}

void ULastFPSCaptureZoneComponent::TickCapture()
{
	if (!bPlayerInside || bCaptured)
	{
		return;
	}
	ElapsedSeconds = FMath::Min(ElapsedSeconds + UpdateInterval, CaptureDuration);
	BroadcastProgress();

	if (ElapsedSeconds >= CaptureDuration)
	{
		bCaptured = true;
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimerHandle);
		OnCaptureComplete.Broadcast(ZoneTag);

		// 퀘스트 통지는 C++ 에서 직접 — BP 브릿지 불필요. ZoneTag 없으면 퀘스트와 무관한 점령으로 취급.
		if (ZoneTag.IsValid())
		{
			if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
			{
				Quest->NotifyObjectiveCaptured(ZoneTag);
			}
		}
		UE_LOG(LogLastFPSCapture, Log, TEXT("[Capture] 점령 완료 (Zone=%s)"), *ZoneTag.ToString());
	}
}

void ULastFPSCaptureZoneComponent::OnRep_Elapsed()
{
	BroadcastProgress();
}

void ULastFPSCaptureZoneComponent::BroadcastProgress()
{
	OnCaptureProgress.Broadcast(GetProgress01());
}

float ULastFPSCaptureZoneComponent::GetProgress01() const
{
	return CaptureDuration > 0.f ? FMath::Clamp(ElapsedSeconds / CaptureDuration, 0.f, 1.f) : 0.f;
}
