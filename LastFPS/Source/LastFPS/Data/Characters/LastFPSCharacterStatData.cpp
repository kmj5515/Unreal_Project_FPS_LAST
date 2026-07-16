#include "Data/Characters/LastFPSCharacterStatData.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"

bool ULastFPSCharacterStatData::ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return false;
	}

	const float ClampedMaxHealth = FMath::Max(MaxHealth, 1.f);
	const float ClampedMaxStamina = FMath::Max(MaxStamina, 1.f);

	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMaxHealthAttribute(), ClampedMaxHealth);
	ASC->SetNumericAttributeBase(
		ULastFPSAttributeSet::GetHealthAttribute(),
		FMath::Clamp(Health, 0.f, ClampedMaxHealth));

	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMaxStaminaAttribute(), ClampedMaxStamina);
	ASC->SetNumericAttributeBase(
		ULastFPSAttributeSet::GetStaminaAttribute(),
		FMath::Clamp(Stamina, 0.f, ClampedMaxStamina));

	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetAttackDamageAttribute(), FMath::Max(AttackDamage, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetAttackRangeAttribute(), FMath::Max(AttackRange, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetCriticalChanceAttribute(), FMath::Clamp(CriticalChance, 0.f, 100.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetCriticalDamagePercentAttribute(), FMath::Max(CriticalDamagePercent, 100.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetDefenseAttribute(), FMath::Max(Defense, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetPhysicalDamageMultiplierAttribute(), FMath::Max(PhysicalDamageMultiplier, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetFireDamageMultiplierAttribute(), FMath::Max(FireDamageMultiplier, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetIceDamageMultiplierAttribute(), FMath::Max(IceDamageMultiplier, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetElectricDamageMultiplierAttribute(), FMath::Max(ElectricDamageMultiplier, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetPoisonDamageMultiplierAttribute(), FMath::Max(PoisonDamageMultiplier, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMoveSpeedAttribute(), FMath::Max(MoveSpeed, 0.f));

	return true;
}
