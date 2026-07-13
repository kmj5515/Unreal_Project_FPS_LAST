#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectTypes.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_MoveSpeedBuff::ULastFPSGE_MoveSpeedBuff()
{
    DurationPolicy  = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));

    StackingType = EGameplayEffectStackingType::AggregateByTarget;
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
