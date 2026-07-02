#include "AbilitySystem/Abilities/GA_ViolaFrostStorm.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystem/Effects/GE_StatusFreeze.h"
#include "AbilitySystem/Effects/GE_StatusSlow.h"
#include "AbilitySystem/Effects/GE_UltimateCooldown.h"
#include "Character/LastFPSHero.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Utility/LastFPSTags.h"

UGA_ViolaFrostStorm::UGA_ViolaFrostStorm()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	CooldownGameplayEffectClass = ULastFPSGE_UltimateCooldown::StaticClass();
	bDrawDebug = true;

	FrostStormEffectEventTag = LastFPSGameplayTags::Event_Montage_ViolaFrostStormEffect;
	AreaEffectClass = ALastFPSAreaEffectActor::StaticClass();
	AreaConfig.Shape = ELastFPSAreaEffectShape::Cone;
	AreaConfig.Radius = 1200.f;
	AreaConfig.ConeAngleDegrees = 75.f;
	AreaConfig.Duration = 0.35f;
	AreaConfig.DamageInterval = 1.f;
	AreaConfig.DamageEffect = ULastFPSGE_DamageInstant::StaticClass();
	AreaConfig.DamageRange.MinDamage = 55.f;
	AreaConfig.DamageRange.MaxDamage = 75.f;
	AreaConfig.DamageRange.DamageElement = ELastFPSDamageElement::Ice;
	AreaConfig.TargetEffects.Add(ULastFPSGE_StatusSlow::StaticClass());
	AreaConfig.TargetEffects.Add(ULastFPSGE_StatusFreeze::StaticClass());
	AreaConfig.VisualRadius = 1200.f;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Ultimate);
	Tags.AddTag(LastFPSGameplayTags::Input_Ultimate);
	SetAssetTags(Tags);
}

bool UGA_ViolaFrostStorm::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ALastFPSHero* Hero = ActorInfo ? Cast<ALastFPSHero>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Hero && Hero->IsAlive();
}

void UGA_ViolaFrostStorm::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero || !Hero->IsAlive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bFrostStormSpawned = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Hero->SetCombatState(EMMCombatState::Casting);
	StartEffectEventTask();

	if (!PlayFrostStormMontage())
	{
		SpawnFrostStorm(Hero);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_ViolaFrostStorm::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	EndEffectEventTask();

	if (FrostStormMontageTask)
	{
		FrostStormMontageTask->EndTask();
		FrostStormMontageTask = nullptr;
	}

	ReleaseCastingState();
	bFrostStormSpawned = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_ViolaFrostStorm::PlayFrostStormMontage()
{
	if (!FrostStormMontage)
	{
		return false;
	}

	FrostStormMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		FrostStormMontage,
		MontagePlayRate);
	if (!FrostStormMontageTask)
	{
		return false;
	}

	FrostStormMontageTask->OnCompleted.AddDynamic(this, &UGA_ViolaFrostStorm::OnFrostStormMontageCompleted);
	FrostStormMontageTask->OnCancelled.AddDynamic(this, &UGA_ViolaFrostStorm::OnFrostStormMontageCancelled);
	FrostStormMontageTask->OnInterrupted.AddDynamic(this, &UGA_ViolaFrostStorm::OnFrostStormMontageInterrupted);
	FrostStormMontageTask->ReadyForActivation();
	return true;
}

void UGA_ViolaFrostStorm::StartEffectEventTask()
{
	if (!FrostStormEffectEventTag.IsValid())
	{
		return;
	}

	FrostStormEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		FrostStormEffectEventTag,
		nullptr,
		true,
		true);
	if (!FrostStormEffectEventTask)
	{
		return;
	}

	FrostStormEffectEventTask->EventReceived.AddDynamic(this, &UGA_ViolaFrostStorm::OnFrostStormEffectEvent);
	FrostStormEffectEventTask->ReadyForActivation();
}

void UGA_ViolaFrostStorm::EndEffectEventTask()
{
	if (FrostStormEffectEventTask)
	{
		FrostStormEffectEventTask->EndTask();
		FrostStormEffectEventTask = nullptr;
	}
}

FVector UGA_ViolaFrostStorm::GetAimDirection(const ALastFPSHero* Hero) const
{
	if (!Hero)
	{
		return FVector::ForwardVector;
	}

	if (const AController* Controller = Hero->GetController())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		FVector AimDirection = ViewRotation.Vector();
		AimDirection.Z = 0.f;
		return AimDirection.GetSafeNormal(SMALL_NUMBER, Hero->GetActorForwardVector());
	}

	FVector AimDirection = Hero->GetActorForwardVector();
	AimDirection.Z = 0.f;
	return AimDirection.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
}

FTransform UGA_ViolaFrostStorm::BuildSpawnTransform(const ALastFPSHero* Hero) const
{
	if (!Hero)
	{
		return FTransform::Identity;
	}

	const FVector AimDirection = GetAimDirection(Hero);
	return FTransform(AimDirection.Rotation(), Hero->GetActorLocation());
}

FLastFPSAreaEffectConfig UGA_ViolaFrostStorm::BuildAreaConfig() const
{
	FLastFPSAreaEffectConfig Config = AreaConfig;
	if (!Config.DamageEffect)
	{
		Config.DamageEffect = ULastFPSGE_DamageInstant::StaticClass();
	}

	Config.bDrawDebug = ShouldDrawDebug();
	Config.DebugDrawTime = GetDebugDrawTime();
	Config.DebugColor = DebugColor;
	return Config;
}

void UGA_ViolaFrostStorm::SpawnFrostStorm(ALastFPSHero* Hero)
{
	if (bFrostStormSpawned)
	{
		return;
	}

	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<ALastFPSAreaEffectActor> AreaClass = AreaEffectClass
		? AreaEffectClass.Get()
		: ALastFPSAreaEffectActor::StaticClass();
	if (!Hero || !World || !Hero->HasAuthority() || !SourceASC || !AreaClass)
	{
		return;
	}

	const FTransform SpawnTransform = BuildSpawnTransform(Hero);
	ALastFPSAreaEffectActor* AreaActor = World->SpawnActorDeferred<ALastFPSAreaEffectActor>(
		AreaClass,
		SpawnTransform,
		Hero,
		Hero,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!AreaActor)
	{
		return;
	}

	bFrostStormSpawned = true;
	AreaActor->InitializeAreaEffect(Hero, SourceASC, BuildAreaConfig());
	AreaActor->FinishSpawning(SpawnTransform);

	DrawDebugLine(
		GetCurrentActorInfo(),
		SpawnTransform.GetLocation(),
		SpawnTransform.GetLocation() + SpawnTransform.GetUnitAxis(EAxis::X) * AreaConfig.Radius);
}

void UGA_ViolaFrostStorm::SpawnFrostStormFromCurrentAvatar()
{
	SpawnFrostStorm(Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()));
}

void UGA_ViolaFrostStorm::ReleaseCastingState()
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (Hero->GetCombatState() == EMMCombatState::Casting)
		{
			Hero->SetCombatState(EMMCombatState::Idle);
		}
	}
}

void UGA_ViolaFrostStorm::OnFrostStormEffectEvent(FGameplayEventData)
{
	SpawnFrostStormFromCurrentAvatar();
}

void UGA_ViolaFrostStorm::OnFrostStormMontageCompleted()
{
	if (!bFrostStormSpawned)
	{
		SpawnFrostStormFromCurrentAvatar();
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_ViolaFrostStorm::OnFrostStormMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_ViolaFrostStorm::OnFrostStormMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
