#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_Skill1, "Cooldown.Skill1");

ULastFPSGE_Skill1Cooldown::ULastFPSGE_Skill1Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));

    FInheritedTagContainer Tags;
    Tags.Added.AddTag(TAG_Cooldown_Skill1);
    InheritableOwnedTagsContainer = Tags;
}
