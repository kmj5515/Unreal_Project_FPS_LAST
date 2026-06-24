#include "AbilitySystem/Effects/GE_StatusFreeze.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectTypes.h"

ULastFPSGE_StatusFreeze::ULastFPSGE_StatusFreeze()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.25f));

    StackingType = EGameplayEffectStackingType::AggregateByTarget;
    StackLimitCount = 1;
    StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    FGameplayModifierInfo Mod;
    Mod.Attribute = ULastFPSAttributeSet::GetMoveSpeedAttribute();
    Mod.ModifierOp = EGameplayModOp::Override;
    Mod.ModifierMagnitude = FScalableFloat(0.f);
    Modifiers.Add(Mod);
}
