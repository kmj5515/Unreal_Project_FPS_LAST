#include "AbilitySystem/Abilities/GA_ViolaIceAura.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"
#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/LastFPSHero.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Pooling/LastFPSActorPoolSpawn.h"
#include "Utility/LastFPSTags.h"

UGA_ViolaIceAura::UGA_ViolaIceAura()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	CooldownGameplayEffectClass = ULastFPSGE_Skill2Cooldown::StaticClass();

	AuraEffectEventTag = LastFPSGameplayTags::Event_Montage_ViolaIceAuraEffect;
	AuraAreaEffectClass = ALastFPSAreaEffectActor::StaticClass();
	DamageRange.DamageElement = ELastFPSDamageElement::Ice;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Skill2);
	Tags.AddTag(LastFPSGameplayTags::Input_Skill2);
	SetAssetTags(Tags);

	ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
}

bool UGA_ViolaIceAura::CanActivateAbility(
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

	return ActorInfo && ActorInfo->AvatarActor.IsValid();
}

void UGA_ViolaIceAura::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	bAuraEffectCommitted = false;
	
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		Hero->SetCombatState(EMMCombatState::Casting);
	}
	
	if (AuraEffectEventTag.IsValid())
	{
		AuraEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			AuraEffectEventTag,
			nullptr,
			true,
			true);
		if (AuraEffectEventTask)
		{
			AuraEffectEventTask->EventReceived.AddDynamic(this, &UGA_ViolaIceAura::OnAuraEffectEvent);
			AuraEffectEventTask->ReadyForActivation();
		}
	}

	if (!AuraMontage)
	{
		if (!CommitAndApplyAuraEffect())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		ReleaseCastingState();
		return;
	}
	
	AuraMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AuraMontage,
		MontagePlayRate);
	if (!AuraMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AuraMontageTask->OnCompleted.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageCompleted);
	AuraMontageTask->OnCancelled.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageCancelled);
	AuraMontageTask->OnInterrupted.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageInterrupted);
	AuraMontageTask->ReadyForActivation();
}

void UGA_ViolaIceAura::ApplyAuraEffect()
{
	if (!AuraEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(AuraEffect);
	if (Spec.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			Spec);
	}
}

bool UGA_ViolaIceAura::CommitAndApplyAuraEffect()
{
	if (bAuraEffectCommitted)
	{
		return true;
	}

	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return false;
	}

	bAuraEffectCommitted = true;
	ApplyAuraEffect();
	StartAuraTrail();
	StartAuraLoop();
	return true;
}

void UGA_ViolaIceAura::StartAuraLoop()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishAura();
		return;
	}

	SpawnAuraAreaEffect();

	const float EffectiveDamageInterval = GetEffectiveDamageInterval();
	if (EffectiveDamageInterval > 0.f && ShouldSpawnAuraAreaEffect())
	{
		World->GetTimerManager().SetTimer(
			AuraTargetEffectTimerHandle,
			this,
			&UGA_ViolaIceAura::SpawnAuraAreaEffect,
			EffectiveDamageInterval,
			true);
	}

	const float EffectiveAuraDuration = GetEffectiveAuraDuration();
	if (EffectiveAuraDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(
			AuraDurationTimerHandle,
			this,
			&UGA_ViolaIceAura::FinishAura,
			EffectiveAuraDuration,
			false);
	}
	else
	{
		FinishAura();
	}
}

void UGA_ViolaIceAura::SpawnAuraAreaEffect()
{
	if (!ShouldSpawnAuraAreaEffect())
	{
		return;
	}

	ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<ALastFPSAreaEffectActor> AreaClass = AuraAreaEffectClass
		? AuraAreaEffectClass.Get()
		: ALastFPSAreaEffectActor::StaticClass();
	if (!Hero || !World || !Hero->HasAuthority() || !SourceASC || !AreaClass)
	{
		return;
	}

	const FTransform SpawnTransform = BuildAuraAreaSpawnTransform(Hero);
	const FLastFPSAreaEffectConfig AuraConfig = BuildAuraAreaConfig();
	ALastFPSAreaEffectActor* AreaActor =
		LastFPSActorPool::AcquireOrSpawnDeferred<ALastFPSAreaEffectActor>(
			*World,
			AreaClass,
			SpawnTransform,
			Hero,
			Hero,
			[Hero, SourceASC, AuraConfig](ALastFPSAreaEffectActor& Actor)
			{
				Actor.InitializeAreaEffect(Hero, SourceASC, AuraConfig);
			});
	if (!AreaActor)
	{
		return;
	}
}

bool UGA_ViolaIceAura::ShouldSpawnAuraAreaEffect() const
{
	return GetEffectiveAuraAreaDuration() > 0.f
		&& GetEffectiveAuraRadius() > 0.f
		&& (AuraDamageEffect || !AuraTargetEffects.IsEmpty() || AuraPulseNiagaraSystem);
}

FLastFPSAreaEffectConfig UGA_ViolaIceAura::BuildAuraAreaConfig() const
{
	FLastFPSAreaEffectConfig AreaConfig;
	AreaConfig.Radius = GetEffectiveAuraRadius();
	AreaConfig.Duration = GetEffectiveAuraAreaDuration();
	AreaConfig.DamageInterval = GetEffectiveDamageInterval();
	AreaConfig.DamageEffect = AuraDamageEffect;
	AreaConfig.DamageCooldownEffect = AuraDamageCooldownEffect;
	AreaConfig.DamageRange = GetEffectiveDamageRange();
	AreaConfig.TargetEffects = AuraTargetEffects;
	AreaConfig.RequiredTargetTags = RequiredTargetTags;
	AreaConfig.BlockedTargetTags = BlockedTargetTags;
	AreaConfig.EffectNiagaraSystem = AuraPulseNiagaraSystem;
	AreaConfig.VisualRadius = GetAuraVisualRadius();
	AreaConfig.VisualRadiusNiagaraParameterName = AuraVisualRadiusNiagaraParameterName;
	AreaConfig.DurationNiagaraParameterName = AuraDurationNiagaraParameterName;
	AreaConfig.bDrawDebug = ShouldDrawDebug();
	AreaConfig.DebugDrawTime = AreaConfig.Duration;
	AreaConfig.DebugColor = DebugColor;
	return AreaConfig;
}

FTransform UGA_ViolaIceAura::BuildAuraAreaSpawnTransform(const AActor* SourceActor) const
{
	const FVector Origin = GetAuraOrigin();
	if (!bAlignAuraAreaToGround)
	{
		return FTransform(FRotator::ZeroRotator, Origin);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return FTransform(FRotator::ZeroRotator, Origin);
	}

	const FVector TraceStart = Origin + FVector::UpVector * AuraGroundTraceStartOffset;
	const FVector TraceEnd = Origin - FVector::UpVector * AuraGroundTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ViolaIceAuraGroundTrace), false, SourceActor);
	if (SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, AuraGroundTraceChannel, QueryParams))
	{
		return FTransform(FRotator::ZeroRotator, Origin);
	}

	const float SurfaceOffset = FMath::Max(AuraGroundSurfaceOffset, AuraGroundMinimumSurfaceOffset);
	const FVector SurfaceLocation = Hit.ImpactPoint + Hit.ImpactNormal * SurfaceOffset;
	const FRotator SurfaceRotation = FRotationMatrix::MakeFromZ(Hit.ImpactNormal).Rotator();
	return FTransform(SurfaceRotation, SurfaceLocation);
}

void UGA_ViolaIceAura::StartAuraTrail()
{
	StopAuraTrail();

	if (!AuraTrailNiagaraSystem)
	{
		return;
	}

	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = Hero ? Hero->GetMesh() : nullptr;
	if (!MeshComponent)
	{
		return;
	}

	const FName AttachBoneName = AuraTrailAttachBoneName.IsNone() ? AuraOriginBoneName : AuraTrailAttachBoneName;
	AuraTrailNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		AuraTrailNiagaraSystem,
		MeshComponent,
		AttachBoneName,
		AuraTrailAttachLocationOffset,
		AuraTrailAttachRotationOffset,
		EAttachLocation::KeepRelativeOffset,
		false,
		false,
		ENCPoolMethod::None,
		true);
	if (!AuraTrailNiagaraComponent)
	{
		return;
	}

	UpdateAuraTrailState();

	if (bDeactivateAuraTrailWhenStationary)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AuraTrailStateTimerHandle,
				this,
				&UGA_ViolaIceAura::UpdateAuraTrailState,
				FMath::Max(AuraTrailStateUpdateInterval, 0.02f),
				true);
		}
	}
}

void UGA_ViolaIceAura::StopAuraTrail()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AuraTrailStateTimerHandle);
	}

	if (AuraTrailNiagaraComponent)
	{
		AuraTrailNiagaraComponent->Deactivate();
		AuraTrailNiagaraComponent->DestroyComponent();
		AuraTrailNiagaraComponent = nullptr;
	}
}

void UGA_ViolaIceAura::UpdateAuraTrailState()
{
	if (!AuraTrailNiagaraComponent)
	{
		return;
	}

	const bool bShouldActivateTrail =
		!bDeactivateAuraTrailWhenStationary ||
		GetAuraTrailSpeed() >= AuraTrailMinActivationSpeed;
	if (bShouldActivateTrail)
	{
		if (!AuraTrailNiagaraComponent->IsActive())
		{
			AuraTrailNiagaraComponent->Activate(true);
		}
		return;
	}

	if (AuraTrailNiagaraComponent->IsActive())
	{
		AuraTrailNiagaraComponent->Deactivate();
	}
}

float UGA_ViolaIceAura::GetAuraTrailSpeed() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->GetVelocity().Size2D() : 0.f;
}

void UGA_ViolaIceAura::FinishAura()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_ViolaIceAura::ReleaseCastingState()
{
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (Hero->GetCombatState() == EMMCombatState::Casting)
		{
			Hero->SetCombatState(EMMCombatState::Idle);
		}
	}
}

FVector UGA_ViolaIceAura::GetCurrentAvatarLocation() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->GetActorLocation() : FVector::ZeroVector;
}

FVector UGA_ViolaIceAura::GetAuraSourceLocation() const
{
	const ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* MeshComponent = Hero ? Hero->GetMesh() : nullptr;
	if (!MeshComponent || AuraOriginBoneName.IsNone())
	{
		return GetCurrentAvatarLocation();
	}

	const bool bHasSocketOrBone =
		MeshComponent->DoesSocketExist(AuraOriginBoneName) ||
		MeshComponent->GetBoneIndex(AuraOriginBoneName) != INDEX_NONE;
	return bHasSocketOrBone ? MeshComponent->GetSocketLocation(AuraOriginBoneName) : GetCurrentAvatarLocation();
}

FVector UGA_ViolaIceAura::GetAuraOrigin() const
{
	return GetAuraSourceLocation();
}

float UGA_ViolaIceAura::GetAuraVisualRadius() const
{
	return AuraVisualRadius > 0.f ? AuraVisualRadius : GetEffectiveAuraRadius() * 2.f;
}

float UGA_ViolaIceAura::GetEffectiveAuraDuration() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	return BalanceData && BalanceData->Duration > 0.f ? BalanceData->Duration : AuraDuration;
}

float UGA_ViolaIceAura::GetEffectiveAuraRadius() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	return BalanceData && BalanceData->Radius > 0.f ? BalanceData->Radius : AuraRadius;
}

float UGA_ViolaIceAura::GetEffectiveAuraAreaDuration() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	const float Value = BalanceData
		? BalanceData->GetParameter(LastFPSGameplayTags::Skill_Parameter_AreaDuration, AuraAreaDuration)
		: AuraAreaDuration;
	return FMath::Max(Value, 0.f);
}

float UGA_ViolaIceAura::GetEffectiveDamageInterval() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	const float Value = BalanceData
		? BalanceData->GetParameter(LastFPSGameplayTags::Skill_Parameter_DamageInterval, DamageInterval)
		: DamageInterval;
	return FMath::Max(Value, 0.f);
}

FLastFPSDamageRange UGA_ViolaIceAura::GetEffectiveDamageRange() const
{
	const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
	return BalanceData && BalanceData->Damage > 0.f
		? LastFPSDamage::MakeDamageRange(
			BalanceData->Damage + GetEquippedWeaponBaseDamage(),
			DamageRange.DamageElement)
		: DamageRange;
}

void UGA_ViolaIceAura::OnAuraEffectEvent(FGameplayEventData)
{
	if (!CommitAndApplyAuraEffect())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
	}
}

void UGA_ViolaIceAura::OnAuraMontageCompleted()
{
	ReleaseCastingState();

	if (!bAuraEffectCommitted)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UGA_ViolaIceAura::OnAuraMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_ViolaIceAura::OnAuraMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_ViolaIceAura::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AuraEffectEventTask)
	{
		AuraEffectEventTask->EndTask();
		AuraEffectEventTask = nullptr;
	}

	if (AuraMontageTask)
	{
		AuraMontageTask->EndTask();
		AuraMontageTask = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AuraTargetEffectTimerHandle);
		World->GetTimerManager().ClearTimer(AuraDurationTimerHandle);
	}

	StopAuraTrail();
	ReleaseCastingState();
	
	bAuraEffectCommitted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
