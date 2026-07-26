#include "Encounter/LastFPSRoomSpawnPresentationComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
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
	PendingSpawnVFXTransforms.Reset();
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

void ULastFPSRoomSpawnPresentationComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	CancelNiagaraSystemLoad();
	PendingSpawnVFXTransforms.Reset();
	LoadedNiagaraSystem = nullptr;
	Super::EndPlay(EndPlayReason);
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
		constexpr int32 MaxPendingVFXCount = 32;
		if (PendingSpawnVFXTransforms.Num() < MaxPendingVFXCount)
		{
			PendingSpawnVFXTransforms.Add(SpawnTransform);
		}
		if (!NiagaraSystemLoadHandle.IsValid())
		{
			RefreshLoadedSystem();
		}
		return;
	}

	PlayLoadedSpawnVFX(SpawnTransform);
}

void ULastFPSRoomSpawnPresentationComponent::PlayLoadedSpawnVFX(
	const FTransform& SpawnTransform)
{
	if (!LoadedNiagaraSystem)
	{
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
	CancelNiagaraSystemLoad();
	LoadedNiagaraSystem = nullptr;
	if (GetNetMode() == NM_DedicatedServer)
	{
		PendingSpawnVFXTransforms.Reset();
		return;
	}
	if (SpawnVFX.NiagaraSystem.IsNull())
	{
		// 초기 프로퍼티 복제보다 RPC가 먼저 처리된 경우 OnRep에서 다시 로드를 시도한다.
		return;
	}

	LoadedNiagaraSystem = SpawnVFX.NiagaraSystem.Get();
	if (LoadedNiagaraSystem)
	{
		HandleNiagaraSystemLoaded();
		return;
	}

	const FSoftObjectPath NiagaraSystemPath = SpawnVFX.NiagaraSystem.ToSoftObjectPath();
	NiagaraSystemLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		NiagaraSystemPath,
		FStreamableDelegate::CreateUObject(
			this,
			&ULastFPSRoomSpawnPresentationComponent::HandleNiagaraSystemLoaded),
		FStreamableManager::AsyncLoadHighPriority);
	if (!NiagaraSystemLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSRoomSpawnPresentation,
			Error,
			TEXT("[%s] 생성 Niagara System 비동기 로드를 시작하지 못했습니다: %s"),
			*GetNameSafe(GetOwner()),
			*NiagaraSystemPath.ToString());
		PendingSpawnVFXTransforms.Reset();
	}
}

void ULastFPSRoomSpawnPresentationComponent::HandleNiagaraSystemLoaded()
{
	LoadedNiagaraSystem = SpawnVFX.NiagaraSystem.Get();
	NiagaraSystemLoadHandle.Reset();
	if (!LoadedNiagaraSystem)
	{
		UE_LOG(
			LogLastFPSRoomSpawnPresentation,
			Error,
			TEXT("[%s] 생성 Niagara System 비동기 로드 결과가 유효하지 않습니다: %s"),
			*GetNameSafe(GetOwner()),
			*SpawnVFX.NiagaraSystem.ToString());
		PendingSpawnVFXTransforms.Reset();
		return;
	}

	TArray<FTransform> PendingTransforms = MoveTemp(PendingSpawnVFXTransforms);
	for (const FTransform& PendingTransform : PendingTransforms)
	{
		PlayLoadedSpawnVFX(PendingTransform);
	}
}

void ULastFPSRoomSpawnPresentationComponent::CancelNiagaraSystemLoad()
{
	if (NiagaraSystemLoadHandle.IsValid())
	{
		NiagaraSystemLoadHandle->CancelHandle();
		NiagaraSystemLoadHandle.Reset();
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
