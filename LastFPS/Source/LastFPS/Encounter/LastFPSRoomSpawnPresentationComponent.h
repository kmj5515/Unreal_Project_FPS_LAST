#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "LastFPSRoomSpawnPresentationComponent.generated.h"

class UNiagaraSystem;
struct FStreamableHandle;

/** 룸 인카운터의 일시적인 생성 연출과 네트워크 전달만 담당한다. */
UCLASS()
class LASTFPS_API ULastFPSRoomSpawnPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSRoomSpawnPresentationComponent();

	void Configure(const FLastFPSRoomEncounterSpawnVFXDefinition& InSpawnVFX);
	void PlaySpawnVFX(const FTransform& SpawnTransform);

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnRep_SpawnVFX();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySpawnVFX(const FTransform& SpawnTransform);

	void RefreshLoadedSystem();
	void HandleNiagaraSystemLoaded();
	void CancelNiagaraSystemLoad();
	void PlayLoadedSpawnVFX(const FTransform& SpawnTransform);
	FTransform MakeVFXTransform(const FTransform& SpawnTransform) const;

	UPROPERTY(ReplicatedUsing=OnRep_SpawnVFX)
	FLastFPSRoomEncounterSpawnVFXDefinition SpawnVFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LoadedNiagaraSystem;

	TArray<FTransform> PendingSpawnVFXTransforms;
	TSharedPtr<FStreamableHandle> NiagaraSystemLoadHandle;
};
