#include "Utility/LastFPSTagFunctionLibrary.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CooldownSkill1()
{
    return LastFPSGameplayTags::Cooldown_Skill1;
}

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CooldownSkillDash()
{
    return LastFPSGameplayTags::Cooldown_Skill_Dash;
}

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CooldownSkill2()
{
    return LastFPSGameplayTags::Cooldown_Skill2;
}

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CharacterDead()
{
    return LastFPSGameplayTags::Character_State_Dead;
}

bool ULastFPSTagFunctionLibrary::ActorHasMatchingGameplayTag(AActor* TargetActor, FGameplayTag TagToCheck)
{
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
    {
        if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
        {
            return ASC->HasMatchingGameplayTag(TagToCheck);
        }
    }
    
    return false;
}
