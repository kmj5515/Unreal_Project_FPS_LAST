#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSPassiveGameplayAbility.h"
#include "AbilitySystem/Actors/LastFPSOrbitingProjectileEmitter.h"
#include "GA_ProbePassive.generated.h"

class ALastFPSCharacterBase;
class UWorld;

UCLASS()
class LASTFPS_API UGA_ProbePassive : public ULastFPSPassiveGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ProbePassive();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Probe", meta=(ClampMin="1"))
	int32 MaxProbeCount = 3;

	UPROPERTY(EditDefaultsOnly, Category="Probe")
	TSubclassOf<ALastFPSOrbitingProjectileEmitter> ProbeEmitterClass;

	UPROPERTY(EditDefaultsOnly, Category="Probe")
	FLastFPSOrbitingProjectileEmitterConfig ProbeEmitterConfig;

	UPROPERTY(EditDefaultsOnly, Category="Probe|Trigger")
	FGameplayTagContainer TriggerAbilityTags;

private:
	void StartListeningForAbilityActivations(const FGameplayAbilityActorInfo* ActorInfo);
	void StopListeningForAbilityActivations();
	void OnOwnerAbilityActivated(UGameplayAbility* ActivatedAbility);
	bool ShouldSpawnProbeForAbility(const UGameplayAbility* ActivatedAbility) const;
	bool IsPrimaryListenerInstance() const;
	bool ShouldSkipDuplicateActivation(const UGameplayAbility* ActivatedAbility) const;
	void MarkHandledActivation(UGameplayAbility* ActivatedAbility);
	bool SpawnProbeEmitter(ALastFPSCharacterBase* SourceCharacter) const;
	int32 FindAvailableSlotIndex(UWorld* World, const ALastFPSCharacterBase* SourceCharacter, int32& OutExistingProbeCount) const;

	FDelegateHandle AbilityActivatedDelegateHandle;
	TWeakObjectPtr<UGameplayAbility> LastHandledAbility;
	uint64 LastHandledFrame = 0;
	uint64 LastSpawnFrame = 0;
};
