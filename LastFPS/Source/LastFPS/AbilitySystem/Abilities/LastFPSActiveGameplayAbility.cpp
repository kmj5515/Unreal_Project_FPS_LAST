#include "AbilitySystem/Abilities/LastFPSActiveGameplayAbility.h"

#include "Components/SceneComponent.h"
#include "GameplayEffect.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Kismet/GameplayStatics.h"

ULastFPSActiveGameplayAbility::ULastFPSActiveGameplayAbility()
{
}

void ULastFPSActiveGameplayAbility::PlayActivationSound() const
{
	if (!ActivationSound)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* AvatarActor =
		ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !AvatarActor)
	{
		return;
	}

	USceneComponent* AttachComponent = AvatarActor->GetRootComponent();
	if (!AttachComponent)
	{
		return;
	}

	UGameplayStatics::SpawnSoundAttached(
		ActivationSound,
		AttachComponent,
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		FMath::Max(ActivationSoundVolumeMultiplier, 0.f),
		FMath::Max(ActivationSoundPitchMultiplier, 0.01f));
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
