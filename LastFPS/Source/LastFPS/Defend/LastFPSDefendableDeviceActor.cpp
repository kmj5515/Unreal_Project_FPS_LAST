#include "Defend/LastFPSDefendableDeviceActor.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ALastFPSDefendableDeviceActor::ALastFPSDefendableDeviceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void ALastFPSDefendableDeviceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALastFPSDefendableDeviceActor, Integrity);
}

void ALastFPSDefendableDeviceActor::BeginPlay()
{
	Super::BeginPlay();

	// 초기 내구도는 서버가 세팅 → 복제로 클라 전파. (스폰 시 0 이면 즉시 파괴 오판을 막는다.)
	if (HasAuthority())
	{
		Integrity = MaxIntegrity;
	}
}

void ALastFPSDefendableDeviceActor::ApplyIntegrityDamage(float Amount)
{
	if (!HasAuthority() || bDestroyedBroadcast || Amount <= 0.f)
	{
		return;
	}

	Integrity = FMath::Max(0.f, Integrity - Amount);
	HandleIntegrityUpdated();
	ForceNetUpdate();
}

void ALastFPSDefendableDeviceActor::ResetIntegrity()
{
	if (!HasAuthority())
	{
		return;
	}

	Integrity = MaxIntegrity;
	bDestroyedBroadcast = false;
	HandleIntegrityUpdated();
	ForceNetUpdate();
}

float ALastFPSDefendableDeviceActor::GetIntegrity01() const
{
	return MaxIntegrity > 0.f ? FMath::Clamp(Integrity / MaxIntegrity, 0.f, 1.f) : 0.f;
}

void ALastFPSDefendableDeviceActor::OnRep_Integrity()
{
	HandleIntegrityUpdated();
}

void ALastFPSDefendableDeviceActor::HandleIntegrityUpdated()
{
	OnIntegrityChanged.Broadcast(Integrity, MaxIntegrity);

	if (Integrity <= 0.f && !bDestroyedBroadcast)
	{
		bDestroyedBroadcast = true;
		OnDeviceDestroyed.Broadcast();
	}
}
