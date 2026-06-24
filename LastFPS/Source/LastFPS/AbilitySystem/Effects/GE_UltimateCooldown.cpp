#include "AbilitySystem/Effects/GE_UltimateCooldown.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_UltimateCooldown::ULastFPSGE_UltimateCooldown()
{
    DurationPolicy    = EGameplayEffectDurationType::HasDuration;
    // 궁극기 쿨다운 기본 60초 (기획 튜닝 대상). 스킬(8초)보다 길게.
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(60.f));

    FInheritedTagContainer Tags;
    Tags.AddTag(FLastFPSTags::Get().Cooldown_Ultimate);
    UTargetTagsGameplayEffectComponent* TargetTagsComponent =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
    GEComponents.Add(TargetTagsComponent);
    TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);
}
