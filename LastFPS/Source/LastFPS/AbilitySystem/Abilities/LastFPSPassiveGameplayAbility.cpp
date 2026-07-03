#include "AbilitySystem/Abilities/LastFPSPassiveGameplayAbility.h"

#include "AbilitySystemComponent.h"

ULastFPSPassiveGameplayAbility::ULastFPSPassiveGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULastFPSPassiveGameplayAbility::OnGiveAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivatePassiveAbility(ActorInfo, Spec);
}

void ULastFPSPassiveGameplayAbility::OnAvatarSet(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	TryActivatePassiveAbility(ActorInfo, Spec);
}

void ULastFPSPassiveGameplayAbility::TryActivatePassiveAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (AvatarActor && !AvatarActor->HasAuthority())
	{
		return;
	}

	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
}
