#include "AbilitySystem/Effects/GE_HealInstant.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"

ULastFPSGE_HealInstant::ULastFPSGE_HealInstant()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute           = ULastFPSAttributeSet::GetHealthAttribute();
    Mod.ModifierOp          = EGameplayModOp::Additive;
    Mod.ModifierMagnitude   = FScalableFloat(35.f);
    Modifiers.Add(Mod);
}
