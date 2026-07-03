#include "AbilitySystem/Effects/GE_StatusFreeze.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_StatusFreeze::ULastFPSGE_StatusFreeze()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.25f));

    StackingType = EGameplayEffectStackingType::AggregateByTarget;
    StackLimitCount = 1;
    StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    FInheritedTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Status_Freeze);

    UTargetTagsGameplayEffectComponent* TargetTagsComponent =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
    GEComponents.Add(TargetTagsComponent);
    TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);

    FGameplayModifierInfo Mod;
    Mod.Attribute = ULastFPSAttributeSet::GetMoveSpeedAttribute();
    Mod.ModifierOp = EGameplayModOp::Override;
    Mod.ModifierMagnitude = FScalableFloat(0.f);
    Modifiers.Add(Mod);
}
