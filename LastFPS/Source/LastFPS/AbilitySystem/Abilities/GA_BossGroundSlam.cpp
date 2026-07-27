#include "AbilitySystem/Abilities/GA_BossGroundSlam.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Actors/LastFPSExpandingMeshAttackActor.h"
#include "AbilitySystemComponent.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Data/Enemies/LastFPSBossGroundSlamData.h"
#include "Engine/World.h"
#include "Pooling/LastFPSActorPoolSpawn.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSBossGroundSlam, Log, All);

UGA_BossGroundSlam::UGA_BossGroundSlam()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Boss_GroundSlam);
	SetAssetTags(Tags);
}

bool UGA_BossGroundSlam::CanActivateAbility(
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

	const ALastFPSEnemyCharacter* Enemy =
		ActorInfo ? Cast<ALastFPSEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Enemy
		&& Enemy->IsAlive()
		&& AttackData
		&& AttackData->AttackMontage
		&& AttackData->ImpactEventTag.IsValid()
		&& AttackData->AttackConfig.EffectNiagaraSystem
		&& AttackData->AttackConfig.EndOuterRadius > 0.f
		&& (AttackData->AttackConfig.DamageEffect
			|| !AttackData->AttackConfig.TargetEffects.IsEmpty());
}

void UGA_BossGroundSlam::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	bImpactSpawned = false;
	SourceEnemy = Cast<ALastFPSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!SourceEnemy || !SourceEnemy->HasAuthority() || !SourceEnemy->IsAlive() || !AttackData)
	{
		UE_LOG(LogLastFPSBossGroundSlam, Error,
			TEXT("Ground Slam 활성화 실패: Source=%s, Data=%s, 원인=서버 실행 조건이 유효하지 않습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AttackData));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!StartImpactEventTask() || !StartAttackMontage())
	{
		UE_LOG(LogLastFPSBossGroundSlam, Error,
			TEXT("Ground Slam 몽타주 시작 실패: Source=%s, Data=%s, Montage=%s, EventTag=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AttackData),
			*GetNameSafe(AttackData ? AttackData->AttackMontage : nullptr),
			AttackData ? *AttackData->ImpactEventTag.ToString() : TEXT("Invalid"));
		FinishCurrentAbility(true);
	}
}

void UGA_BossGroundSlam::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	bEndingAbility = true;

	if (ImpactEventTask)
	{
		ImpactEventTask->EndTask();
		ImpactEventTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	SourceEnemy = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_BossGroundSlam::StartImpactEventTask()
{
	if (!AttackData || !AttackData->ImpactEventTag.IsValid())
	{
		return false;
	}

	ImpactEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		AttackData->ImpactEventTag,
		nullptr,
		true,
		true);
	if (!ImpactEventTask)
	{
		return false;
	}

	ImpactEventTask->EventReceived.AddDynamic(this, &UGA_BossGroundSlam::OnImpactEventReceived);
	ImpactEventTask->ReadyForActivation();
	return true;
}

bool UGA_BossGroundSlam::StartAttackMontage()
{
	if (!AttackData || !AttackData->AttackMontage)
	{
		return false;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackData->AttackMontage,
		FMath::Max(AttackData->MontagePlayRate, 0.01f));
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_BossGroundSlam::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_BossGroundSlam::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_BossGroundSlam::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
	return true;
}

void UGA_BossGroundSlam::ExecuteGroundSlam()
{
	if (bEndingAbility || bImpactSpawned || !SourceEnemy || !SourceEnemy->IsAlive() || !AttackData)
	{
		FinishCurrentAbility(true);
		return;
	}

	if (!SpawnExpandingMeshAttack())
	{
		UE_LOG(LogLastFPSBossGroundSlam, Error,
			TEXT("Ground Slam 범위 생성 실패: Source=%s, Data=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AttackData));
		FinishCurrentAbility(true);
		return;
	}

	bImpactSpawned = true;
}

bool UGA_BossGroundSlam::SpawnExpandingMeshAttack()
{
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const TSubclassOf<ALastFPSExpandingMeshAttackActor> AttackActorClass =
		AttackData && AttackData->AttackActorClass
		? AttackData->AttackActorClass.Get()
		: ALastFPSExpandingMeshAttackActor::StaticClass();
	if (!World || !SourceEnemy || !SourceEnemy->HasAuthority() || !SourceASC || !AttackActorClass)
	{
		return false;
	}

	const FTransform SpawnTransform = ResolveGroundTransform();
	ALastFPSExpandingMeshAttackActor* AttackActor =
		LastFPSActorPool::AcquireOrSpawnDeferred<ALastFPSExpandingMeshAttackActor>(
			*World,
			AttackActorClass,
			SpawnTransform,
			SourceEnemy,
			SourceEnemy,
			[this, SourceASC](ALastFPSExpandingMeshAttackActor& Actor)
			{
				Actor.InitializeAttack(
					SourceEnemy,
					SourceASC,
					AttackData->AttackConfig);
			});
	if (!AttackActor)
	{
		return false;
	}
	return true;
}

FTransform UGA_BossGroundSlam::ResolveGroundTransform() const
{
	if (!SourceEnemy || !AttackData)
	{
		return FTransform::Identity;
	}

	FVector GroundLocation = SourceEnemy->GetActorLocation();
	if (const UCapsuleComponent* Capsule = SourceEnemy->GetCapsuleComponent())
	{
		GroundLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();
	}

	FVector SurfaceNormal = FVector::UpVector;
	if (AttackData->bProjectToGround)
	{
		if (UWorld* World = SourceEnemy->GetWorld())
		{
			const FVector TraceStart = SourceEnemy->GetActorLocation()
				+ FVector::UpVector * FMath::Max(AttackData->GroundTraceStartOffset, 0.f);
			const FVector TraceEnd = SourceEnemy->GetActorLocation()
				- FVector::UpVector * FMath::Max(AttackData->GroundTraceDistance, 0.f);
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossGroundSlamGround), false, SourceEnemy);
			QueryParams.AddIgnoredActor(SourceEnemy);

			FHitResult GroundHit;
			if (World->LineTraceSingleByChannel(
				GroundHit,
				TraceStart,
				TraceEnd,
				AttackData->GroundTraceChannel,
				QueryParams))
			{
				SurfaceNormal = GroundHit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
				GroundLocation = GroundHit.ImpactPoint
					+ SurfaceNormal * FMath::Max(AttackData->GroundSurfaceOffset, 0.f);
			}
		}
	}

	return FTransform(FRotationMatrix::MakeFromZ(SurfaceNormal).Rotator(), GroundLocation);
}

void UGA_BossGroundSlam::OnImpactEventReceived(FGameplayEventData Payload)
{
	if (!AttackData || !Payload.EventTag.MatchesTagExact(AttackData->ImpactEventTag))
	{
		return;
	}

	ExecuteGroundSlam();
}

void UGA_BossGroundSlam::FinishCurrentAbility(const bool bWasCancelled)
{
	if (bEndingAbility || !IsActive())
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

void UGA_BossGroundSlam::OnMontageCompleted()
{
	if (!bImpactSpawned)
	{
		UE_LOG(LogLastFPSBossGroundSlam, Warning,
			TEXT("Ground Slam 몽타주가 충격 이벤트 없이 종료되었습니다: Source=%s, Montage=%s, EventTag=%s"),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AttackData ? AttackData->AttackMontage : nullptr),
			AttackData ? *AttackData->ImpactEventTag.ToString() : TEXT("Invalid"));
	}

	FinishCurrentAbility(!bImpactSpawned);
}

void UGA_BossGroundSlam::OnMontageCancelled()
{
	FinishCurrentAbility(true);
}

void UGA_BossGroundSlam::OnMontageInterrupted()
{
	FinishCurrentAbility(true);
}
