#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "NativeGameplayTags.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_Skill1Cooldown::ULastFPSGE_Skill1Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));

    FInheritedTagContainer Tags;
    Tags.Added.AddTag(FLastFPSTags::Get().Cooldown_Skill1);
    InheritableOwnedTagsContainer = Tags;
}
