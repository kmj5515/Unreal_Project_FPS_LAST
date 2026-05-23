#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
#include "NativeGameplayTags.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_Skill2Cooldown::ULastFPSGE_Skill2Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(15.f));

    FInheritedTagContainer Tags;
    Tags.Added.AddTag(FLastFPSTags::Get().Cooldown_Skill2);
    InheritableOwnedTagsContainer = Tags;
}
