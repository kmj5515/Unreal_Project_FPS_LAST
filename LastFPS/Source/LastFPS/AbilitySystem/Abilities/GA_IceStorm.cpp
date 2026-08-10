#include "AbilitySystem/Abilities/GA_IceStorm.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_Cooldown.h"
#include "Animation/AnimInstance.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Pooling/LastFPSActorPoolSpawn.h"
#include "TimerManager.h"
#include "Utility/LastFPSTags.h"

UGA_IceStorm::UGA_IceStorm()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownGameplayEffectClass = ULastFPSGE_Cooldown::StaticClass();

	ConfirmEventTag = LastFPSGameplayTags::Event_Montage_AbilityCommit;
	SpawnEventTag = LastFPSGameplayTags::Event_Montage_IceStormSpawn;
	AbilityEndEventTag = LastFPSGameplayTags::Event_Montage_AbilityEnd;
	ConfirmInputTag = LastFPSGameplayTags::Input_Fire;
	CancelInputTag = LastFPSGameplayTags::Input_ADS;
	AreaEffectClass = ALastFPSAreaEffectActor::StaticClass();
	AreaConfig.DamageRange.DamageElement = ELastFPSDamageElement::Ice;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Skill3);
	Tags.AddTag(LastFPSGameplayTags::Input_Skill3);
	SetAssetTags(Tags);

	ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
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
	CachedTargetTransform = FTransform::Identity;
	bHasCachedTargetTransform = false;

	Hero->SetWantsToSprint(false);
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		FGameplayTagContainer SprintTags;
		SprintTags.AddTag(LastFPSGameplayTags::Input_Sprint);
		ASC->CancelAbilities(&SprintTags);
	}

	Hero->SetCombatState(EMMCombatState::Casting);
	if (UWeaponComponent* WeaponComponent = Hero->GetWeaponComponent())
	{
		WeaponComponent->SetWeaponHiddenForAbility(true);
	}

	StartEventTasks();
	StartTargetingIndicator();

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

bool UGA_IceStorm::CanConfirmAbilityInput(FGameplayTag InputTag) const
{
	const bool bMatchesConfirmInput = !ConfirmInputTag.IsValid() || ConfirmInputTag == InputTag;
	return bMatchesConfirmInput && Phase == ELastFPSIceStormPhase::Casting && !bCommitted;
}

bool UGA_IceStorm::ConfirmAbilityInput(FGameplayTag InputTag)
{
	if (!CanConfirmAbilityInput(InputTag))
	{
		return false;
	}

	return ConfirmIceStorm();
}

bool UGA_IceStorm::CanCancelAbilityInput(FGameplayTag InputTag) const
{
	const bool bMatchesCancelInput = CancelInputTag.IsValid() && CancelInputTag == InputTag;
	return bMatchesCancelInput && Phase == ELastFPSIceStormPhase::Casting && !bCommitted;
}

bool UGA_IceStorm::CancelAbilityInput(FGameplayTag InputTag)
{
	if (!CanCancelAbilityInput(InputTag))
	{
		return false;
	}

	CancelIceStorm();
	return true;
}

bool UGA_IceStorm::ShouldBlockAbilityInputRelease(FGameplayTag InputTag) const
{
	const bool bOwnsInputTag = GetAssetTags().HasTagExact(InputTag);
	return bOwnsInputTag && Phase == ELastFPSIceStormPhase::Casting && !bCommitted;
}

bool UGA_IceStorm::ConfirmIceStorm()
{
	if (Phase != ELastFPSIceStormPhase::Casting || bCommitted)
	{
		return false;
	}

	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	if (!CacheAimTarget())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return true;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return true;
	}

	bCommitted = true;
	Phase = ELastFPSIceStormPhase::Executing;
	StopTargetingIndicator();
	DrawTargetDebug();

	if (!JumpToMontageSection(FireSectionName))
	{
		SpawnAreaEffect();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	return true;
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

void UGA_IceStorm::StopIceStormMontage() const
{
	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!IceStormMontage || !Hero || !Hero->GetMesh())
	{
		return;
	}

	UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance();
	if (AnimInstance && AnimInstance->Montage_IsPlaying(IceStormMontage))
	{
		AnimInstance->Montage_Stop(CancelMontageBlendOutTime, IceStormMontage);
	}
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
	CachedTargetTransform = BuildTargetGroundTransform(Hero, CachedTargetLocation);
	CachedTargetLocation = CachedTargetTransform.GetLocation();
	bHasCachedTargetTransform = true;
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
	const FVector TraceEnd = ViewLocation + TraceDirection * GetEffectiveAimTraceRange();

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

float UGA_IceStorm::GetEffectiveAimTraceRange() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	return BalanceData && BalanceData->Range > 0.f ? BalanceData->Range : AimTraceRange;
}

FLastFPSAreaEffectConfig UGA_IceStorm::BuildAreaConfig() const
{
	FLastFPSAreaEffectConfig Config = AreaConfig;
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	if (!BalanceData)
	{
		return Config;
	}

	if (BalanceData->Radius > 0.f)
	{
		const float VisualRadiusRatio = AreaConfig.Radius > 0.f && AreaConfig.VisualRadius > 0.f
			? AreaConfig.VisualRadius / AreaConfig.Radius
			: 2.f;
		Config.Radius = BalanceData->Radius;
		Config.VisualRadius = BalanceData->Radius * VisualRadiusRatio;
	}
	if (BalanceData->Duration > 0.f)
	{
		Config.Duration = BalanceData->Duration;
	}
	Config.DamageInterval = FMath::Max(
		BalanceData->GetParameter(
			LastFPSGameplayTags::Skill_Parameter_DamageInterval,
			Config.DamageInterval),
		0.f);
	if (BalanceData->Damage > 0.f)
	{
		Config.DamageRange = LastFPSDamage::MakeDamageRange(
			BalanceData->Damage + GetEquippedWeaponBaseDamage(),
			AreaConfig.DamageRange.DamageElement);
	}
	return Config;
}

FTransform UGA_IceStorm::BuildTargetGroundTransform(const ALastFPSHero* Hero, const FVector& TargetLocation) const
{
	if (!bProjectTargetToGround)
	{
		return FTransform(FRotator::ZeroRotator, TargetLocation);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return FTransform(FRotator::ZeroRotator, TargetLocation);
	}

	const FVector TraceStart = TargetLocation + FVector::UpVector * GroundTraceStartOffset;
	const FVector TraceEnd = TargetLocation - FVector::UpVector * GroundTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IceStormGroundTrace), false, Hero);
	if (Hero)
	{
		QueryParams.AddIgnoredActor(Hero);

		TArray<AActor*> AttachedActors;
		Hero->GetAttachedActors(AttachedActors);
		QueryParams.AddIgnoredActors(AttachedActors);
	}

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GroundTraceChannel, QueryParams))
	{
		return FTransform(FRotator::ZeroRotator, TargetLocation);
	}

	const FVector SurfaceLocation = HitResult.ImpactPoint + HitResult.ImpactNormal * GroundSurfaceOffset;
	const FRotator SurfaceRotation = FRotationMatrix::MakeFromZ(HitResult.ImpactNormal).Rotator();
	return FTransform(SurfaceRotation, SurfaceLocation);
}

void UGA_IceStorm::StartTargetingIndicator()
{
	StopTargetingIndicator();

	if (!ShouldShowTargetingIndicator() || !TargetingIndicatorNiagaraSystem)
	{
		return;
	}

	UpdateTargetingIndicator();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TargetingIndicatorTimerHandle,
			this,
			&UGA_IceStorm::UpdateTargetingIndicator,
			TargetingIndicatorUpdateInterval,
			true);
	}
}

void UGA_IceStorm::UpdateTargetingIndicator()
{
	if (!ShouldShowTargetingIndicator() || !TargetingIndicatorNiagaraSystem)
	{
		StopTargetingIndicator();
		return;
	}

	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	if (!Hero)
	{
		StopTargetingIndicator();
		return;
	}

	const FVector CameraAimDirection = GetCameraAimDirection(Hero);
	const FVector AimTarget = GetAimTarget(Hero, CameraAimDirection);
	const FTransform GroundTransform = BuildTargetGroundTransform(Hero, AimTarget);
	const FTransform IndicatorTransform = BuildTargetingIndicatorTransform(GroundTransform);

	if (!TargetingIndicatorNiagaraComponent)
	{
		TargetingIndicatorNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			TargetingIndicatorNiagaraSystem,
			IndicatorTransform.GetLocation(),
			IndicatorTransform.GetRotation().Rotator(),
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::None,
			true);
	}

	if (!TargetingIndicatorNiagaraComponent)
	{
		return;
	}

	TargetingIndicatorNiagaraComponent->SetWorldTransform(IndicatorTransform);
	ApplyTargetingIndicatorParameters(IndicatorTransform);
}

void UGA_IceStorm::StopTargetingIndicator()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TargetingIndicatorTimerHandle);
	}

	if (TargetingIndicatorNiagaraComponent)
	{
		TargetingIndicatorNiagaraComponent->Deactivate();
		TargetingIndicatorNiagaraComponent->DestroyComponent();
		TargetingIndicatorNiagaraComponent = nullptr;
	}
}

bool UGA_IceStorm::ShouldShowTargetingIndicator() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo
		&& ActorInfo->IsLocallyControlled()
		&& Phase == ELastFPSIceStormPhase::Casting
		&& !bCommitted;
}

FTransform UGA_IceStorm::BuildTargetingIndicatorTransform(const FTransform& GroundTransform) const
{
	const FVector IndicatorLocation = GroundTransform.TransformPosition(TargetingIndicatorLocationOffset);
	const FQuat IndicatorRotation = GroundTransform.GetRotation() * TargetingIndicatorRotationOffset.Quaternion();
	return FTransform(IndicatorRotation, IndicatorLocation, TargetingIndicatorScale);
}

void UGA_IceStorm::ApplyTargetingIndicatorParameters(const FTransform& IndicatorTransform)
{
	if (!TargetingIndicatorNiagaraComponent)
	{
		return;
	}

	const FLastFPSAreaEffectConfig EffectiveAreaConfig = BuildAreaConfig();
	if (!TargetingIndicatorRadiusNiagaraParameterName.IsNone())
	{
		TargetingIndicatorNiagaraComponent->SetVariableFloat(TargetingIndicatorRadiusNiagaraParameterName, EffectiveAreaConfig.Radius);
	}

	if (!TargetingIndicatorVisualRadiusNiagaraParameterName.IsNone())
	{
		const float VisualRadius = EffectiveAreaConfig.VisualRadius > 0.f
			? EffectiveAreaConfig.VisualRadius
			: EffectiveAreaConfig.Radius * 2.f;
		TargetingIndicatorNiagaraComponent->SetVariableFloat(TargetingIndicatorVisualRadiusNiagaraParameterName, VisualRadius);
	}

	if (!TargetingIndicatorSurfaceNormalNiagaraParameterName.IsNone())
	{
		TargetingIndicatorNiagaraComponent->SetVariableVec3(TargetingIndicatorSurfaceNormalNiagaraParameterName, IndicatorTransform.GetUnitAxis(EAxis::Z));
	}
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

	const FTransform SpawnTransform = bHasCachedTargetTransform
		? CachedTargetTransform
		: FTransform(FRotator::ZeroRotator, CachedTargetLocation);
	ALastFPSAreaEffectActor* AreaActor =
		LastFPSActorPool::AcquireOrSpawnDeferred<ALastFPSAreaEffectActor>(
			*World,
			AreaClass,
			SpawnTransform,
			Hero,
			Hero,
			[Hero, SourceASC, this](ALastFPSAreaEffectActor& Actor)
			{
				Actor.InitializeAreaEffect(Hero, SourceASC, BuildAreaConfig());
			});
	if (!AreaActor)
	{
		return;
	}

	bAreaSpawned = true;
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
	StopTargetingIndicator();
	ReleaseCastingState();

	if (bWasCancelled)
	{
		StopIceStormMontage();
	}

	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (UWeaponComponent* WeaponComponent = Hero->GetWeaponComponent())
		{
			WeaponComponent->SetWeaponHiddenForAbility(false);
		}
	}

	Phase = ELastFPSIceStormPhase::None;
	bCommitted = false;
	bAreaSpawned = false;
	CachedTargetLocation = FVector::ZeroVector;
	CachedTargetTransform = FTransform::Identity;
	bHasCachedTargetTransform = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
