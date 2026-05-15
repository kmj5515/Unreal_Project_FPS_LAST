#include "AbilitySystem/Effects/GE_UltimateKillHeal.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSPlayerState.h"

ULastFPSGE_UltimateKillHeal::ULastFPSGE_UltimateKillHeal()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute         = ULastFPSAttributeSet::GetHealthAttribute();
    Mod.ModifierOp        = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FScalableFloat(ALastFPSPlayerState::UltimateKillHealAmount);
    Modifiers.Add(Mod);
}
