#include "Defend/LastFPSDefendObjectiveComponent.h"

#include "Defend/LastFPSDefendableDeviceActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSDefend, Log, All);

ULastFPSDefendObjectiveComponent::ULastFPSDefendObjectiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULastFPSDefendObjectiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULastFPSDefendObjectiveComponent, ElapsedSeconds);
}

void ULastFPSDefendObjectiveComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart && HasAuthority())
	{
		StartDefense();
	}
}

void ULastFPSDefendObjectiveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}
	if (bBoundToDevice && IsValid(Device))
	{
		Device->OnDeviceDestroyed.RemoveDynamic(this, &ULastFPSDefendObjectiveComponent::HandleDeviceDestroyed);
	}
	bBoundToDevice = false;

	Super::EndPlay(EndPlayReason);
}

bool ULastFPSDefendObjectiveComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

ULastFPSQuestSubsystem* ULastFPSDefendObjectiveComponent::GetQuestSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>() : nullptr;
}

void ULastFPSDefendObjectiveComponent::StartDefense()
{
	if (!HasAuthority() || bActive)
	{
		return;
	}

	// Device 미지정 시, 컴포넌트가 붙은 액터 자체가 장치면 그걸 대상으로(같은 액터에 얹는 흔한 구성).
	if (!IsValid(Device))
	{
		Device = Cast<ALastFPSDefendableDeviceActor>(GetOwner());
	}
	if (!IsValid(Device))
	{
		UE_LOG(LogLastFPSDefend, Error,
			TEXT("[%s] 지킬 대상(Device)이 지정되지 않아 방어를 시작할 수 없습니다."),
			*GetNameSafe(GetOwner()));
		return;
	}

	bActive = true;
	ElapsedSeconds = 0.f;

	if (!bBoundToDevice)
	{
		Device->OnDeviceDestroyed.AddDynamic(this, &ULastFPSDefendObjectiveComponent::HandleDeviceDestroyed);
		bBoundToDevice = true;
	}
	Device->ResetIntegrity(); // 시작 시 대상 내구도 만충으로 재무장

	GetWorld()->GetTimerManager().SetTimer(
		HoldTimerHandle, this, &ULastFPSDefendObjectiveComponent::TickHold, UpdateInterval, true);
	BroadcastProgress();
}

void ULastFPSDefendObjectiveComponent::StopDefense()
{
	if (!HasAuthority())
	{
		return;
	}
	bActive = false;
	GetWorld()->GetTimerManager().ClearTimer(HoldTimerHandle);
}

void ULastFPSDefendObjectiveComponent::TickHold()
{
	if (!bActive)
	{
		return;
	}
	ElapsedSeconds = FMath::Min(ElapsedSeconds + UpdateInterval, HoldDuration);
	BroadcastProgress();

	if (ElapsedSeconds >= HoldDuration)
	{
		Succeed();
	}
}

void ULastFPSDefendObjectiveComponent::Succeed()
{
	StopDefense();
	OnDefenseSucceeded.Broadcast(ZoneTag);

	// 퀘스트 통지는 C++ 에서 직접 — BP 브릿지 불필요. ZoneTag 없으면 퀘스트와 무관한 방어로 취급.
	if (ZoneTag.IsValid())
	{
		if (ULastFPSQuestSubsystem* Quest = GetQuestSubsystem())
		{
			Quest->NotifyObjectiveDefended(ZoneTag);
		}
	}
	UE_LOG(LogLastFPSDefend, Log, TEXT("[Defend] 방어 성공 (Zone=%s)"), *ZoneTag.ToString());
}

void ULastFPSDefendObjectiveComponent::Fail()
{
	StopDefense();
	OnDefenseFailed.Broadcast(ZoneTag);
	UE_LOG(LogLastFPSDefend, Log, TEXT("[Defend] 방어 실패 - 대상 파괴 (Zone=%s)"), *ZoneTag.ToString());

	// 최소 리셋: 대상 내구도 복구 후 재시작(StartDefense 가 ResetIntegrity 수행). 더 큰 리스타트는 델리게이트로 위임.
	if (bRestartOnFail)
	{
		StartDefense();
	}
}

void ULastFPSDefendObjectiveComponent::HandleDeviceDestroyed()
{
	if (bActive)
	{
		Fail();
	}
}

void ULastFPSDefendObjectiveComponent::OnRep_Elapsed()
{
	BroadcastProgress();
}

void ULastFPSDefendObjectiveComponent::BroadcastProgress()
{
	OnDefenseProgress.Broadcast(GetProgress01());
}

float ULastFPSDefendObjectiveComponent::GetProgress01() const
{
	return HoldDuration > 0.f ? FMath::Clamp(ElapsedSeconds / HoldDuration, 0.f, 1.f) : 0.f;
}
