#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"

#include "GameplayEffect.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"

ULastFPSActiveGameplayAbility::ULastFPSActiveGameplayAbility()
{
}

void ULastFPSActiveGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (!BalanceData || BalanceData->Cooldown <= 0.f || !CooldownEffect)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownEffect->GetClass(),
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpec.IsValid())
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	CooldownSpec.Data->SetDuration(BalanceData->Cooldown, true);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
}
