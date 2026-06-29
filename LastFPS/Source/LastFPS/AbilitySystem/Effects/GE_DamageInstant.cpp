#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_DamageInstant::ULastFPSGE_DamageInstant()
{
    ConfigureDamageModifier();
}

void ULastFPSGE_DamageInstant::PostLoad()
{
    Super::PostLoad();
    ConfigureDamageModifier();
}

void ULastFPSGE_DamageInstant::ConfigureDamageModifier()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    Modifiers.Reset();

    FGameplayModifierInfo Mod;
    Mod.Attribute         = ULastFPSAttributeSet::GetDamageAttribute();
    Mod.ModifierOp        = EGameplayModOp::Additive;

    FSetByCallerFloat DamageMagnitude;
    DamageMagnitude.DataTag = LastFPSGameplayTags::SetByCaller_Damage;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);

    Modifiers.Add(Mod);
}
