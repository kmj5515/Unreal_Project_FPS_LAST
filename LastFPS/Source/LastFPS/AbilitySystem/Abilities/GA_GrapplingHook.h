#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"
#include "GA_GrapplingHook.generated.h"

class ALastFPSHero;
class UPrimitiveComponent;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilityTask_WaitDelay;
class ULastFPSGrapplingHookData;

UCLASS()
class LASTFPS_API UGA_GrapplingHook : public ULastFPSActiveGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_GrapplingHook();

	virtual void OnAvatarSet(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

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

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grappling Hook")
	TObjectPtr<ULastFPSGrapplingHookData> GrapplingData;

private:
	void StartHookFlight(ALastFPSHero& Hero);
	void StartAttachedPull(ALastFPSHero& Hero);
	void StartGrapplePull();
	void StartWireGameplayCue(ALastFPSHero& Hero, const FVector& SurfaceNormal);

	UFUNCTION()
	void OnHookAttached();

	UFUNCTION()
	void OnAttachDelayFinished();

	UFUNCTION()
	void OnGrappleMovementFinished();

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> GrappleMovementTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> HookFlightTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttachDelayTask;

	FVector GrappleAnchor = FVector::ZeroVector;
	FVector GrapplePullDestination = FVector::ZeroVector;
	FVector GrappleSurfaceNormal = FVector::UpVector;
	TWeakObjectPtr<UPrimitiveComponent> GrappleTargetComponent;
	bool bPullStarted = false;
	bool bGrapplingAnimationStateStarted = false;
	bool bCameraEffectStarted = false;
	bool bHookAttached = false;
	bool bWireGameplayCueActive = false;
};
