#include "AbilitySystem/Effects/GE_StatusSlow.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_StatusSlow::ULastFPSGE_StatusSlow()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    // UE 5.7의 SetStackingType은 Editor 전용 비공개 심볼이라 게임 모듈에서 링크할 수 없다.
    StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    StackLimitCount = 1;
    StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    FInheritedTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Status_Movement_Slow);

    UTargetTagsGameplayEffectComponent* TargetTagsComponent =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
    GEComponents.Add(TargetTagsComponent);
    TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);

    FGameplayModifierInfo Mod;
    Mod.Attribute = ULastFPSAttributeSet::GetMoveSpeedAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FScalableFloat(-250.f);
    Modifiers.Add(Mod);
}
