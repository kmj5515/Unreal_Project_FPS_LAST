#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_Skill2, "Cooldown.Skill2");

ULastFPSGE_Skill2Cooldown::ULastFPSGE_Skill2Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(15.f));

    FInheritedTagContainer Tags;
    Tags.Added.AddTag(TAG_Cooldown_Skill2);
    InheritableOwnedTagsContainer = Tags;
}
