#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_Skill2Cooldown::ULastFPSGE_Skill2Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(15.f));

    FInheritedTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Cooldown_Skill2);
    UTargetTagsGameplayEffectComponent* TargetTagsComponent =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
    GEComponents.Add(TargetTagsComponent);
    TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);
}
