#include "Utility/LastFPSDamageCalculation.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Utility/LastFPSTags.h"

namespace
{
	constexpr float DamageVarianceRatio = 0.1f;
}

float FLastFPSDamageRange::Roll() const
{
	return LastFPSDamage::RollDamage(*this);
}

FLastFPSDamageRange LastFPSDamage::MakeDamageRange(
	const float BaseDamage,
	const ELastFPSDamageElement DamageElement)
{
	const float SafeBaseDamage = FMath::Max(BaseDamage, 0.f);

	FLastFPSDamageRange DamageRange;
	DamageRange.MinDamage = SafeBaseDamage * (1.f - DamageVarianceRatio);
	DamageRange.MaxDamage = SafeBaseDamage * (1.f + DamageVarianceRatio);
	DamageRange.DamageElement = DamageElement;
	return DamageRange;
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

namespace
{
float GetElementDamageMultiplier(const ULastFPSAttributeSet& SourceAttributes, ELastFPSDamageElement DamageElement)
{
	switch (DamageElement)
	{
	case ELastFPSDamageElement::Fire:
		return SourceAttributes.GetFireDamageMultiplier();
	case ELastFPSDamageElement::Ice:
		return SourceAttributes.GetIceDamageMultiplier();
	case ELastFPSDamageElement::Electric:
		return SourceAttributes.GetElectricDamageMultiplier();
	case ELastFPSDamageElement::Poison:
		return SourceAttributes.GetPoisonDamageMultiplier();
	case ELastFPSDamageElement::Physical:
	default:
		return SourceAttributes.GetPhysicalDamageMultiplier();
	}
}
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
	Result.DamageAmount *= FMath::Max(GetElementDamageMultiplier(*SourceAttributes, DamageRange.DamageElement), 0.f);

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
	if (!EffectClass)
	{
		return false;
	}

	const UGameplayEffect* Effect = EffectClass->GetDefaultObject<UGameplayEffect>();
	if (!Effect)
	{
		return false;
	}

	const FGameplayAttribute DamageAttribute = ULastFPSAttributeSet::GetDamageAttribute();
	for (const FGameplayModifierInfo& Modifier : Effect->Modifiers)
	{
		if (Modifier.Attribute != DamageAttribute)
		{
			continue;
		}

		if (Modifier.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::SetByCaller)
		{
			continue;
		}

		const FSetByCallerFloat& SetByCaller = Modifier.ModifierMagnitude.GetSetByCallerFloat();
		if (SetByCaller.DataTag == LastFPSGameplayTags::SetByCaller_Damage)
		{
			return true;
		}
	}

	return false;
}

void LastFPSDamage::RollAndApplySetByCallerDamage(
	FGameplayEffectSpec& Spec,
	const FLastFPSDamageRange& DamageRange)
{
	ApplySetByCallerDamage(Spec, CalculateDamage(Spec, DamageRange));
}
