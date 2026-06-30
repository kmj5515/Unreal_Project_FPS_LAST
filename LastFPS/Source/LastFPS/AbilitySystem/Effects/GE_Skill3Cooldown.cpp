#include "AbilitySystem/Effects/GE_Skill3Cooldown.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Utility/LastFPSTags.h"

ULastFPSGE_Skill3Cooldown::ULastFPSGE_Skill3Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(20.f));

	FInheritedTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Cooldown_Skill3);

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
	GEComponents.Add(TargetTagsComponent);
	TargetTagsComponent->SetAndApplyTargetTagChanges(Tags);
}
