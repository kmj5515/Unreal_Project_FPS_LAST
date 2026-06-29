#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
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
	const TArray<TSubclassOf<UGameplayEffect>>& Effects,
	const FLastFPSDamageRange& DamageRange)
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
			LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), DamageRange);
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void ULastFPSProjectileImpactRule::DrawDebugPoint(
	const FLastFPSProjectileImpactContext& Context,
	const FVector& Location) const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawDebug)
	{
		return;
	}

	const AActor* WorldActor = Context.ProjectileActor.Get() ? Context.ProjectileActor.Get() : Context.SourceActor.Get();
	UWorld* World = WorldActor ? WorldActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	::DrawDebugPoint(
		World,
		Location,
		DebugPointSize,
		DebugColor.ToFColor(true),
		false,
		DebugDrawTime);
#endif
}

void ULastFPSProjectileImpactRule::DrawDebugSphere(
	const FLastFPSProjectileImpactContext& Context,
	const FVector& Location,
	float Radius) const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawDebug || Radius <= 0.f)
	{
		return;
	}

	const AActor* WorldActor = Context.ProjectileActor.Get() ? Context.ProjectileActor.Get() : Context.SourceActor.Get();
	UWorld* World = WorldActor ? WorldActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	::DrawDebugSphere(
		World,
		Location,
		Radius,
		24,
		DebugColor.ToFColor(true),
		false,
		DebugDrawTime,
		0,
		DebugLineThickness);
#endif
}

void ULastFPSProjectileImpactRule::DrawDebugLine(
	const FLastFPSProjectileImpactContext& Context,
	const FVector& Start,
	const FVector& End) const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawDebug)
	{
		return;
	}

	const AActor* WorldActor = Context.ProjectileActor.Get() ? Context.ProjectileActor.Get() : Context.SourceActor.Get();
	UWorld* World = WorldActor ? WorldActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	::DrawDebugLine(
		World,
		Start,
		End,
		DebugColor.ToFColor(true),
		false,
		DebugDrawTime,
		0,
		DebugLineThickness);
#endif
}
