#include "Character/Components/LastFPSCombatAimComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSCombatAim, Log, All);

ULastFPSCombatAimComponent::ULastFPSCombatAimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULastFPSCombatAimComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULastFPSCombatAimComponent, bAimActive);
	DOREPLIFETIME(ULastFPSCombatAimComponent, AimTargetLocation);
	DOREPLIFETIME(ULastFPSCombatAimComponent, bFiringActive);
	DOREPLIFETIME(ULastFPSCombatAimComponent, ShotSequence);
}

void ULastFPSCombatAimComponent::SetAimTarget(UObject* RequestOwner, const FVector& TargetLocation)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		UE_LOG(LogLastFPSCombatAim, Error,
			TEXT("조준 상태 갱신 실패: Component=%s, Owner=%s, 원인=서버 권한이 없습니다."),
			*GetNameSafe(this),
			*GetNameSafe(OwnerActor));
		return;
	}

	if (!IsValid(RequestOwner) || TargetLocation.ContainsNaN())
	{
		UE_LOG(LogLastFPSCombatAim, Error,
			TEXT("조준 상태 갱신 실패: Owner=%s, RequestOwner=%s, Target=%s, 원인=유효하지 않은 입력입니다."),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(RequestOwner),
			*TargetLocation.ToCompactString());
		return;
	}

	ActiveRequestOwner = RequestOwner;
	AimTargetLocation = TargetLocation;
	bAimActive = true;
	OwnerActor->ForceNetUpdate();
}

void ULastFPSCombatAimComponent::ClearAimTarget(UObject* RequestOwner)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (ActiveRequestOwner.IsValid() && ActiveRequestOwner.Get() != RequestOwner)
	{
		return;
	}

	ActiveRequestOwner.Reset();
	bAimActive = false;
	OwnerActor->ForceNetUpdate();
}

void ULastFPSCombatAimComponent::BeginFiring(UObject* RequestOwner)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !IsValid(RequestOwner))
	{
		UE_LOG(LogLastFPSCombatAim, Error,
			TEXT("발사 상태 시작 실패: Owner=%s, RequestOwner=%s, 원인=서버 권한 또는 유효한 요청 소유자가 없습니다."),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(RequestOwner));
		return;
	}

	ActiveFiringRequestOwner = RequestOwner;
	bFiringActive = true;
	OwnerActor->ForceNetUpdate();
}

void ULastFPSCombatAimComponent::NotifyShotFired(UObject* RequestOwner)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !bFiringActive
		|| !ActiveFiringRequestOwner.IsValid() || ActiveFiringRequestOwner.Get() != RequestOwner)
	{
		return;
	}

	ShotSequence = ShotSequence == TNumericLimits<int32>::Max() ? 1 : ShotSequence + 1;
	OwnerActor->ForceNetUpdate();
}

void ULastFPSCombatAimComponent::EndFiring(UObject* RequestOwner)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (ActiveFiringRequestOwner.IsValid() && ActiveFiringRequestOwner.Get() != RequestOwner)
	{
		return;
	}

	ActiveFiringRequestOwner.Reset();
	bFiringActive = false;
	OwnerActor->ForceNetUpdate();
}
