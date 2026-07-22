#include "Encounter/LastFPSRoomSpawnPresentationComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRoomSpawnPresentation, Log, All);

ULastFPSRoomSpawnPresentationComponent::ULastFPSRoomSpawnPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULastFPSRoomSpawnPresentationComponent::Configure(
	const FLastFPSRoomEncounterSpawnVFXDefinition& InSpawnVFX)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(
			LogLastFPSRoomSpawnPresentation,
			Error,
			TEXT("[%s] 생성 연출 설정은 서버에서만 변경할 수 있습니다."),
			*GetNameSafe(Owner));
		return;
	}

	SpawnVFX = InSpawnVFX;
	RefreshLoadedSystem();
	Owner->ForceNetUpdate();
}

void ULastFPSRoomSpawnPresentationComponent::PlaySpawnVFX(
	const FTransform& SpawnTransform)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || SpawnVFX.NiagaraSystem.IsNull())
	{
		return;
	}

	MulticastPlaySpawnVFX(SpawnTransform);
}

void ULastFPSRoomSpawnPresentationComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULastFPSRoomSpawnPresentationComponent, SpawnVFX);
}

void ULastFPSRoomSpawnPresentationComponent::OnRep_SpawnVFX()
{
	RefreshLoadedSystem();
}

void ULastFPSRoomSpawnPresentationComponent::MulticastPlaySpawnVFX_Implementation(
	const FTransform& SpawnTransform)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!LoadedNiagaraSystem)
	{
		RefreshLoadedSystem();
	}

	if (!LoadedNiagaraSystem)
	{
		UE_LOG(
			LogLastFPSRoomSpawnPresentation,
			Warning,
			TEXT("[%s] 생성 Niagara System을 불러오지 못해 연출을 재생하지 않습니다: %s"),
			*GetNameSafe(GetOwner()),
			*SpawnVFX.NiagaraSystem.ToString());
		return;
	}

	const FTransform VFXTransform = MakeVFXTransform(SpawnTransform);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		LoadedNiagaraSystem,
		VFXTransform.GetLocation(),
		VFXTransform.Rotator(),
		VFXTransform.GetScale3D(),
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);
}

void ULastFPSRoomSpawnPresentationComponent::RefreshLoadedSystem()
{
	LoadedNiagaraSystem = nullptr;
	if (GetNetMode() != NM_DedicatedServer && !SpawnVFX.NiagaraSystem.IsNull())
	{
		LoadedNiagaraSystem = SpawnVFX.NiagaraSystem.LoadSynchronous();
	}
}

FTransform ULastFPSRoomSpawnPresentationComponent::MakeVFXTransform(
	const FTransform& SpawnTransform) const
{
	const FVector VFXLocation = SpawnTransform.TransformPosition(SpawnVFX.LocationOffset);
	const FQuat VFXRotation =
		SpawnTransform.GetRotation() * SpawnVFX.RotationOffset.Quaternion();
	return FTransform(VFXRotation, VFXLocation, SpawnVFX.Scale);
}
