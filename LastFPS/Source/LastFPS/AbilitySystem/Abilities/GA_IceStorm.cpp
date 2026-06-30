#include "AbilitySystem/Abilities/GA_IceStorm.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_Skill3Cooldown.h"
#include "Animation/AnimInstance.h"
#include "Character/LastFPSHero.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Utility/LastFPSTags.h"

UGA_IceStorm::UGA_IceStorm()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownGameplayEffectClass = ULastFPSGE_Skill3Cooldown::StaticClass();

	ConfirmEventTag = LastFPSGameplayTags::Event_Montage_AbilityCommit;
	SpawnEventTag = LastFPSGameplayTags::Event_Montage_IceStormSpawn;
	AbilityEndEventTag = LastFPSGameplayTags::Event_Montage_AbilityEnd;
	AreaEffectClass = ALastFPSAreaEffectActor::StaticClass();
	AreaConfig.DamageRange.DamageElement = ELastFPSDamageElement::Ice;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Skill3);
	Tags.AddTag(LastFPSGameplayTags::Input_Skill3);
	SetAssetTags(Tags);
}

bool UGA_IceStorm::CanActivateAbility(
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
	return Hero && Hero->IsAlive() && Hero->GetCombatState() == EMMCombatState::Idle;
}

void UGA_IceStorm::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero || !Hero->IsAlive() || Hero->GetCombatState() != EMMCombatState::Idle)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bCommitted = false;
	bAreaSpawned = false;
	Phase = ELastFPSIceStormPhase::Casting;
	CachedTargetLocation = FVector::ZeroVector;

	Hero->SetWantsToSprint(false);
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		FGameplayTagContainer SprintTags;
		SprintTags.AddTag(LastFPSGameplayTags::Input_Sprint);
		ASC->CancelAbilities(&SprintTags);
	}

	Hero->SetCombatState(EMMCombatState::Casting);
	StartEventTasks();

	if (!PlayIceStormMontage())
	{
		ConfirmIceStorm();
	}
}

void UGA_IceStorm::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (Phase == ELastFPSIceStormPhase::Casting)
	{
		ConfirmIceStorm();
	}
}

void UGA_IceStorm::ConfirmIceStorm()
{
	if (Phase != ELastFPSIceStormPhase::Casting || bCommitted)
	{
		return;
	}

	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	if (!CacheAimTarget())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bCommitted = true;
	Phase = ELastFPSIceStormPhase::Executing;
	DrawTargetDebug();

	if (!JumpToMontageSection(FireSectionName))
	{
		SpawnAreaEffect();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_IceStorm::CancelIceStorm()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_IceStorm::StartEventTasks()
{
	if (ConfirmEventTag.IsValid())
	{
		ConfirmEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ConfirmEventTag, nullptr, true, true);
		if (ConfirmEventTask)
		{
			ConfirmEventTask->EventReceived.AddDynamic(this, &UGA_IceStorm::OnConfirmEvent);
			ConfirmEventTask->ReadyForActivation();
		}
	}

	if (SpawnEventTag.IsValid())
	{
		SpawnEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SpawnEventTag, nullptr, true, true);
		if (SpawnEventTask)
		{
			SpawnEventTask->EventReceived.AddDynamic(this, &UGA_IceStorm::OnSpawnEvent);
			SpawnEventTask->ReadyForActivation();
		}
	}

	if (AbilityEndEventTag.IsValid())
	{
		AbilityEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AbilityEndEventTag, nullptr, true, true);
		if (AbilityEndEventTask)
		{
			AbilityEndEventTask->EventReceived.AddDynamic(this, &UGA_IceStorm::OnAbilityEndEvent);
			AbilityEndEventTask->ReadyForActivation();
		}
	}
}

bool UGA_IceStorm::PlayIceStormMontage()
{
	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!IceStormMontage || !Hero || !Hero->GetMesh())
	{
		return false;
	}

	UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	const float PlayedDuration = AnimInstance->Montage_Play(IceStormMontage, MontagePlayRate);
	if (PlayedDuration <= 0.f)
	{
		return false;
	}

	if (!CastSectionName.IsNone())
	{
		AnimInstance->Montage_JumpToSection(CastSectionName, IceStormMontage);
	}

	return true;
}

bool UGA_IceStorm::JumpToMontageSection(FName SectionName) const
{
	if (SectionName.IsNone())
	{
		return false;
	}

	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!IceStormMontage || !Hero || !Hero->GetMesh())
	{
		return false;
	}

	UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance();
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(IceStormMontage))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(SectionName, IceStormMontage);
	return true;
}

bool UGA_IceStorm::CacheAimTarget()
{
	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		return false;
	}

	const FVector CameraAimDirection = GetCameraAimDirection(Hero);
	CachedTargetLocation = GetAimTarget(Hero, CameraAimDirection);
	return true;
}

FVector UGA_IceStorm::GetCameraAimDirection(const ALastFPSHero* Hero) const
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
		return ViewRotation.Vector().GetSafeNormal();
	}

	return Hero->GetActorForwardVector().GetSafeNormal();
}

FVector UGA_IceStorm::GetAimTarget(const ALastFPSHero* Hero, const FVector& CameraAimDirection) const
{
	if (!Hero)
	{
		return FVector::ZeroVector;
	}

	FVector ViewLocation = Hero->GetActorLocation();
	FRotator ViewRotation = Hero->GetActorRotation();
	if (const AController* Controller = Hero->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceDirection = CameraAimDirection.IsNearlyZero()
		? ViewRotation.Vector().GetSafeNormal()
		: CameraAimDirection.GetSafeNormal();
	const FVector TraceEnd = ViewLocation + TraceDirection * AimTraceRange;

	UWorld* World = GetWorld();
	if (!World)
	{
		return TraceEnd;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IceStormAimTrace), false, Hero);
	QueryParams.AddIgnoredActor(Hero);

	TArray<AActor*> AttachedActors;
	Hero->GetAttachedActors(AttachedActors);
	QueryParams.AddIgnoredActors(AttachedActors);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByObjectType(
		HitResult,
		ViewLocation,
		TraceEnd,
		ObjectParams,
		QueryParams);

	return bHit ? HitResult.ImpactPoint : TraceEnd;
}

void UGA_IceStorm::SpawnAreaEffect()
{
	if (bAreaSpawned || !bCommitted)
	{
		return;
	}

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<ALastFPSAreaEffectActor> AreaClass = AreaEffectClass
		? AreaEffectClass.Get()
		: ALastFPSAreaEffectActor::StaticClass();

	if (!Hero || !World || !Hero->HasAuthority() || !SourceASC || !AreaClass)
	{
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, CachedTargetLocation);
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

	bAreaSpawned = true;
	AreaActor->InitializeAreaEffect(Hero, SourceASC, AreaConfig);
	AreaActor->FinishSpawning(SpawnTransform);
}

void UGA_IceStorm::ReleaseCastingState()
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (Hero->GetCombatState() == EMMCombatState::Casting)
		{
			Hero->SetCombatState(EMMCombatState::Idle);
		}
	}
}

void UGA_IceStorm::EndEventTasks()
{
	if (ConfirmEventTask)
	{
		ConfirmEventTask->EndTask();
		ConfirmEventTask = nullptr;
	}

	if (SpawnEventTask)
	{
		SpawnEventTask->EndTask();
		SpawnEventTask = nullptr;
	}

	if (AbilityEndEventTask)
	{
		AbilityEndEventTask->EndTask();
		AbilityEndEventTask = nullptr;
	}
}

void UGA_IceStorm::DrawTargetDebug() const
{
	DrawDebugPoint(GetCurrentActorInfo(), CachedTargetLocation);

	if (const AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		DrawDebugLine(GetCurrentActorInfo(), AvatarActor->GetActorLocation(), CachedTargetLocation);
	}
}

void UGA_IceStorm::OnConfirmEvent(FGameplayEventData)
{
	ConfirmIceStorm();
}

void UGA_IceStorm::OnSpawnEvent(FGameplayEventData)
{
	if (!bCommitted)
	{
		ConfirmIceStorm();
	}

	SpawnAreaEffect();
}

void UGA_IceStorm::OnAbilityEndEvent(FGameplayEventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_IceStorm::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	EndEventTasks();
	ReleaseCastingState();

	Phase = ELastFPSIceStormPhase::None;
	bCommitted = false;
	bAreaSpawned = false;
	CachedTargetLocation = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
