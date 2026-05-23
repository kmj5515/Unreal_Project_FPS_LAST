#include "Utility/LastFPSTagFunctionLibrary.h"
#include "Utility/LastFPSTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CooldownSkill1()
{
    return FLastFPSTags::Get().Cooldown_Skill1;
}

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CooldownSkill2()
{
    return FLastFPSTags::Get().Cooldown_Skill2;
}

FGameplayTag ULastFPSTagFunctionLibrary::GetTag_CharacterDead()
{
    return FLastFPSTags::Get().Character_State_Dead;
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
