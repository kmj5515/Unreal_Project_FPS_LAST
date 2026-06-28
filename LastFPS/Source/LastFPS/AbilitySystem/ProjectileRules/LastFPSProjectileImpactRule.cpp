#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

UAbilitySystemComponent* ULastFPSProjectileImpactRule::GetAbilitySystemComponent(AActor* Actor)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}

bool ULastFPSProjectileImpactRule::DoesTargetPassTags(
	UAbilitySystemComponent* TargetASC,
	const FGameplayTagContainer& RequiredTags,
	const FGameplayTagContainer& BlockedTags)
{
	if (!TargetASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);

	if (!RequiredTags.IsEmpty() && !OwnedTags.HasAll(RequiredTags))
	{
		return false;
	}

	if (!BlockedTags.IsEmpty() && OwnedTags.HasAny(BlockedTags))
	{
		return false;
	}

	return true;
}

void ULastFPSProjectileImpactRule::ApplyGameplayEffectsToTarget(
	const FLastFPSProjectileImpactContext& Context,
	AActor* TargetActor,
	const TArray<TSubclassOf<UGameplayEffect>>& Effects)
{
	if (!Context.SourceASC || !TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Effects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = Context.SourceASC->MakeEffectContext();
		EffectContext.AddSourceObject(Context.ProjectileActor);
		EffectContext.AddInstigator(Context.SourceActor, Context.ProjectileActor);

		FGameplayEffectSpecHandle Spec = Context.SourceASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
		if (Spec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}
