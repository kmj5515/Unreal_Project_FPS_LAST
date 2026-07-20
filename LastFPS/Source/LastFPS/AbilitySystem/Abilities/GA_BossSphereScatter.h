#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_BossSphereScatter.generated.h"

class ALastFPSEnemyCharacter;
class ULastFPSBossSphereScatterData;

/** 보스 주변의 무작위 착지점으로 구체 투사체를 포물선 발사하는 서버 권한 어빌리티다. */
UCLASS()
class LASTFPS_API UGA_BossSphereScatter : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BossSphereScatter();

	virtual bool CanActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	void SpawnAllSpheres();
	void SpawnNextSphere();
	bool SpawnSphere(int32 SphereIndex);
	FTransform ResolveSpawnBasis() const;
	FVector ResolveSpawnLocation(int32 SphereIndex) const;
	FVector ResolveLandingLocation() const;
	bool ResolveLaunchVelocity(
		const FVector& SpawnLocation,
		const FVector& LandingLocation,
		FVector& OutLaunchVelocity) const;
	void FinishSpawning();
	void FinishRecovery();
	void FinishCurrentAbility(bool bWasCancelled);

	UPROPERTY(EditDefaultsOnly, Category="Boss|Sphere Scatter")
	TObjectPtr<ULastFPSBossSphereScatterData> AttackData;

	UPROPERTY()
	TObjectPtr<ALastFPSEnemyCharacter> SourceEnemy;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	int32 NextSphereIndex = 0;
	int32 SpawnedSphereCount = 0;
	bool bEndingAbility = false;
};
