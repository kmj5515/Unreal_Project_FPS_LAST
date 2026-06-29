#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_Skill1Cooldown::ULastFPSGE_Skill1Cooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(8.f));

    FInheritedTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Cooldown_Skill1);
    UTargetTagsGameplayEffectComponent* TargetTagsComponent =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
    GEComponents.Add(TargetTagsComponent);
    TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);
}
