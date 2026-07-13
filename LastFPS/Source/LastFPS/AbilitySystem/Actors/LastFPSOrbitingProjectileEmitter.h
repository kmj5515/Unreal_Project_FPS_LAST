#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "LastFPSOrbitingProjectileEmitter.generated.h"

class ALastFPSCharacterBase;
class ULastFPSAbilityProjectileData;
class USceneComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSOrbitingProjectileEmitterConfig
{
	GENERATED_BODY()

	FLastFPSOrbitingProjectileEmitterConfig();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter")
	TObjectPtr<ULastFPSAbilityProjectileData> ProjectileData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter", meta=(ClampMin="0.01"))
	float FireInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter")
	FVector RelativeOffset = FVector(30.f, -90.f, 120.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter")
	FVector SlotOffset = FVector(0.f, -42.f, -8.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter")
	TArray<FVector> SlotOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter", meta=(ClampMin="0.0"))
	float FollowInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter")
	bool bFireImmediately = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter|Lifetime", meta=(ClampMin="0"))
	int32 MaxFireCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter|Targeting")
	bool bUseAutoTargeting = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter|Targeting")
	bool bRequireTargetToFire = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter|Targeting", meta=(ClampMin="0.0"))
	float TargetSearchRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emitter|Targeting", meta=(ClampMin="0.0", ClampMax="180.0"))
	float TargetAimConeAngle = 35.f;
};

UCLASS(Blueprintable)
class LASTFPS_API ALastFPSOrbitingProjectileEmitter : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSOrbitingProjectileEmitter();

	void InitializeEmitter(
		ALastFPSCharacterBase* InSourceCharacter,
		int32 InSlotIndex,
		const FLastFPSOrbitingProjectileEmitterConfig& InConfig,
		float InBaseDamageOverride);

	AActor* GetSourceActor() const;
	int32 GetSlotIndex() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Emitter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Emitter")
	TObjectPtr<UStaticMeshComponent> ProbeMesh;

private:
	void RestartFireTimer();
	void UpdateFollowLocation(float DeltaSeconds, bool bSnap);
	void FireProjectile();
	bool CanFireProjectile() const;
	FVector GetSlotOffset() const;
	bool ResolveFireTarget(UWorld* World, const FVector& SpawnLocation, const FVector& CameraAimDirection, FVector& OutTargetLocation) const;
	AActor* FindBestTargetActor(UWorld* World, const FVector& SpawnLocation, const FVector& CameraAimDirection) const;
	bool IsValidTargetActor(const AActor* TargetActor) const;
	FVector GetTargetLocation(const AActor* TargetActor) const;

	UPROPERTY()
	TObjectPtr<ALastFPSCharacterBase> SourceCharacter;

	UPROPERTY()
	FLastFPSOrbitingProjectileEmitterConfig Config;

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	int32 FireCount = 0;

	float BaseDamageOverride = 0.f;

	FTimerHandle FireTimerHandle;
};
