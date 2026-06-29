#include "Utility/LastFPSDamageCalculation.h"

#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Utility/LastFPSTags.h"

float FLastFPSDamageRange::Roll() const
{
	return LastFPSDamage::RollDamage(*this);
}

float LastFPSDamage::RollDamage(const FLastFPSDamageRange& DamageRange)
{
	const float MinDamage = FMath::Max(0.f, DamageRange.MinDamage);
	const float MaxDamage = FMath::Max(MinDamage, DamageRange.MaxDamage);

	if (FMath::IsNearlyEqual(MinDamage, MaxDamage))
	{
		return MinDamage;
	}

	return FMath::FRandRange(MinDamage, MaxDamage);
}

FLastFPSDamageResult LastFPSDamage::CalculateDamage(
	const FGameplayEffectSpec& Spec,
	const FLastFPSDamageRange& DamageRange)
{
	FLastFPSDamageResult Result;
	Result.DamageAmount = RollDamage(DamageRange);

	const UAbilitySystemComponent* SourceASC = Spec.GetEffectContext().GetInstigatorAbilitySystemComponent();
	const ULastFPSAttributeSet* SourceAttributes = SourceASC
		? SourceASC->GetSet<ULastFPSAttributeSet>()
		: nullptr;
	if (!SourceAttributes)
	{
		return Result;
	}

	Result.DamageAmount += FMath::Max(SourceAttributes->GetAttackDamage(), 0.f);

	const float CriticalChance = FMath::Clamp(SourceAttributes->GetCriticalChance(), 0.f, 100.f);
	Result.bCriticalHit = CriticalChance > 0.f && FMath::FRandRange(0.f, 100.f) < CriticalChance;
	if (Result.bCriticalHit)
	{
		const float CriticalDamageMultiplier = FMath::Max(SourceAttributes->GetCriticalDamagePercent(), 100.f) * 0.01f;
		Result.DamageAmount *= CriticalDamageMultiplier;
	}

	return Result;
}

void LastFPSDamage::ApplySetByCallerDamage(FGameplayEffectSpec& Spec, const FLastFPSDamageResult& DamageResult)
{
	Spec.SetSetByCallerMagnitude(LastFPSGameplayTags::SetByCaller_Damage, FMath::Max(DamageResult.DamageAmount, 0.f));
	Spec.SetSetByCallerMagnitude(LastFPSGameplayTags::SetByCaller_CriticalHit, DamageResult.bCriticalHit ? 1.f : 0.f);
}

bool LastFPSDamage::IsDamageGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass)
{
	return EffectClass && EffectClass->IsChildOf(ULastFPSGE_DamageInstant::StaticClass());
}

void LastFPSDamage::RollAndApplySetByCallerDamage(
	FGameplayEffectSpec& Spec,
	TSubclassOf<UGameplayEffect> EffectClass,
	const FLastFPSDamageRange& DamageRange)
{
	if (!IsDamageGameplayEffect(EffectClass))
	{
		return;
	}

	ApplySetByCallerDamage(Spec, CalculateDamage(Spec, DamageRange));
}
