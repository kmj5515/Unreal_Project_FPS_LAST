#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"

ULastFPSGE_DamageInstant::ULastFPSGE_DamageInstant()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute         = ULastFPSAttributeSet::GetDamageAttribute();
    Mod.ModifierOp        = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FScalableFloat(15.f);
    Modifiers.Add(Mod);
}
