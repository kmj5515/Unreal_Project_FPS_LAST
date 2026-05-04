#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectTypes.h"

ULastFPSGE_MoveSpeedBuff::ULastFPSGE_MoveSpeedBuff()
{
    DurationPolicy  = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));

    StackingType = EGameplayEffectStackingType::AggregateByTarget;
    StackLimitCount = 1;
    StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    FGameplayModifierInfo Mod;
    Mod.Attribute         = ULastFPSAttributeSet::GetMoveSpeedAttribute();
    Mod.ModifierOp        = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FScalableFloat(250.f);
    Modifiers.Add(Mod);
}
