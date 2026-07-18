#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectTypes.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_MoveSpeedBuff::ULastFPSGE_MoveSpeedBuff()
{
    DurationPolicy  = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    // UE 5.7의 SetStackingType은 Editor 전용 비공개 심볼이라 게임 모듈에서 링크할 수 없다.
    StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    StackLimitCount = 1;
    StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    FGameplayModifierInfo Mod;
    Mod.Attribute         = ULastFPSAttributeSet::GetMoveSpeedAttribute();
    Mod.ModifierOp        = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FScalableFloat(250.f);
    Modifiers.Add(Mod);

	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(LastFPSGameplayTags::Status_Movement_SpeedBoost);
	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
	GEComponents.Add(TargetTagsComponent);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
