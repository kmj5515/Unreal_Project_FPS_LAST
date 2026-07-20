#include "AbilitySystem/Abilities/GA_BossSphereScatter.h"

#include "Character/LastFPSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Enemies/LastFPSBossSphereScatterData.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Projectiles/LastFPSProjectileLaunchUtility.h"
#include "TimerManager.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSBossSphereScatter, Log, All);

UGA_BossSphereScatter::UGA_BossSphereScatter()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Boss_SphereScatter);
	SetAssetTags(Tags);
}

bool UGA_BossSphereScatter::CanActivateAbility(
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
		&& AttackData->ProjectileData
		&& AttackData->ProjectileData->ProjectileClass
		&& AttackData->ProjectileCount > 0
		&& AttackData->MaximumLandingRadius > 0.f
		&& AttackData->ProjectileGravityScale > 0.f
		&& AttackData->ProjectileLifeSpan > 0.f;
}

void UGA_BossSphereScatter::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	NextSphereIndex = 0;
	SpawnedSphereCount = 0;
	SourceEnemy = Cast<ALastFPSEnemyCharacter>(GetAvatarActorFromActorInfo());

	if (!SourceEnemy || !SourceEnemy->HasAuthority() || !SourceEnemy->IsAlive()
		|| !AttackData || !AttackData->ProjectileData
		|| !AttackData->ProjectileData->ProjectileClass)
	{
		UE_LOG(LogLastFPSBossSphereScatter, Error,
			TEXT("보스 구체 분산 공격 활성화 실패: Source=%s, Data=%s, ProjectileData=%s, 원인=서버 실행 조건 또는 필수 설정이 유효하지 않습니다."),
			*GetNameSafe(SourceEnemy),
			*GetNameSafe(AttackData),
			*GetNameSafe(AttackData ? AttackData->ProjectileData : nullptr));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AttackData->SpawnInterval <= 0.f)
	{
		SpawnAllSpheres();
	}
	else
	{
		SpawnNextSphere();
	}
}

void UGA_BossSphereScatter::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	bEndingAbility = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
	}
	SourceEnemy = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BossSphereScatter::SpawnAllSpheres()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !SourceEnemy->IsAlive())
	{
		FinishCurrentAbility(true);
		return;
	}

	for (int32 SphereIndex = 0; SphereIndex < AttackData->ProjectileCount; ++SphereIndex)
	{
		SpawnedSphereCount += SpawnSphere(SphereIndex) ? 1 : 0;
	}
	NextSphereIndex = AttackData->ProjectileCount;
	FinishSpawning();
}

void UGA_BossSphereScatter::SpawnNextSphere()
{
	if (bEndingAbility || !AttackData || !SourceEnemy || !SourceEnemy->IsAlive())
	{
		FinishCurrentAbility(true);
		return;
	}

	if (NextSphereIndex >= AttackData->ProjectileCount)
	{
		FinishSpawning();
		return;
	}

	SpawnedSphereCount += SpawnSphere(NextSphereIndex) ? 1 : 0;
	++NextSphereIndex;
	if (NextSphereIndex >= AttackData->ProjectileCount)
	{
		FinishSpawning();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&UGA_BossSphereScatter::SpawnNextSphere,
			FMath::Max(AttackData->SpawnInterval, 0.01f),
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

bool UGA_BossSphereScatter::SpawnSphere(const int32 SphereIndex)
{
	if (!SourceEnemy || !AttackData || !AttackData->ProjectileData)
	{
		return false;
	}

	const FVector SpawnLocation = ResolveSpawnLocation(SphereIndex);
	const FVector LandingLocation = ResolveLandingLocation();
	FVector LaunchVelocity = FVector::ZeroVector;
	if (!ResolveLaunchVelocity(SpawnLocation, LandingLocation, LaunchVelocity))
	{
		UE_LOG(LogLastFPSBossSphereScatter, Warning,
			TEXT("보스 구체 포물선 계산 실패: Source=%s, Index=%d, Start=%s, Landing=%s"),
			*GetNameSafe(SourceEnemy),
			SphereIndex,
			*SpawnLocation.ToCompactString(),
			*LandingLocation.ToCompactString());
		return false;
	}

	FLastFPSProjectileLaunchRequest LaunchRequest;
	LaunchRequest.SourceActor = SourceEnemy;
	LaunchRequest.ProjectileData = AttackData->ProjectileData;
	LaunchRequest.AimTarget = LandingLocation;
	LaunchRequest.FallbackAimDirection = LaunchVelocity.GetSafeNormal();
	LaunchRequest.bOverrideSpawnLocation = true;
	LaunchRequest.SpawnLocationOverride = SpawnLocation;
	LaunchRequest.bApplyProjectileDataSpawnOffset = false;
	LaunchRequest.bOverrideLaunchVelocity = true;
	LaunchRequest.LaunchVelocityOverride = LaunchVelocity;
	LaunchRequest.bOverrideGravityScale = true;
	LaunchRequest.GravityScaleOverride = AttackData->ProjectileGravityScale;
	LaunchRequest.LifeSpanOverride = AttackData->ProjectileLifeSpan;

	ALastFPSProjectile* Projectile = LastFPSProjectileLaunch::SpawnProjectile(LaunchRequest);
	if (!Projectile)
	{
		UE_LOG(LogLastFPSBossSphereScatter, Warning,
			TEXT("보스 구체 생성 실패: Source=%s, Index=%d, ProjectileData=%s"),
			*GetNameSafe(SourceEnemy),
			SphereIndex,
			*GetNameSafe(AttackData->ProjectileData));
		return false;
	}

#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(GetCurrentActorInfo(), SpawnLocation, 20.f);
	DrawDebugSphere(GetCurrentActorInfo(), LandingLocation, 35.f);
#endif
	return true;
}

FTransform UGA_BossSphereScatter::ResolveSpawnBasis() const
{
	if (!SourceEnemy || !AttackData)
	{
		return FTransform::Identity;
	}

	FTransform SpawnBasis = SourceEnemy->GetActorTransform();
	if (const USkeletalMeshComponent* Mesh = SourceEnemy->GetMesh();
		Mesh && !AttackData->SpawnSocketName.IsNone()
			&& Mesh->DoesSocketExist(AttackData->SpawnSocketName))
	{
		SpawnBasis = Mesh->GetSocketTransform(AttackData->SpawnSocketName, RTS_World);
	}
	SpawnBasis.SetScale3D(FVector::OneVector);
	return SpawnBasis;
}

FVector UGA_BossSphereScatter::ResolveSpawnLocation(const int32 SphereIndex) const
{
	const FTransform SpawnBasis = ResolveSpawnBasis();
	FVector LocalOffset = AttackData ? AttackData->SpawnOriginOffset : FVector::ZeroVector;
	if (AttackData && AttackData->ProjectileSpawnOffsets.IsValidIndex(SphereIndex))
	{
		LocalOffset += AttackData->ProjectileSpawnOffsets[SphereIndex].Offset;
	}
	if (AttackData && AttackData->SpawnSeparationRadius > 0.f)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::Sqrt(FMath::FRand()) * AttackData->SpawnSeparationRadius;
		LocalOffset += FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
	}
	return SpawnBasis.TransformPositionNoScale(LocalOffset);
}

FVector UGA_BossSphereScatter::ResolveLandingLocation() const
{
	if (!SourceEnemy || !AttackData)
	{
		return FVector::ZeroVector;
	}

	const FTransform SourceTransform = SourceEnemy->GetActorTransform();
	const FVector LandingCenter = SourceTransform.TransformPositionNoScale(AttackData->LandingCenterOffset);
	const float MinimumRadius = FMath::Clamp(
		AttackData->MinimumLandingRadius,
		0.f,
		FMath::Max(AttackData->MaximumLandingRadius, 0.f));
	const float MaximumRadius = FMath::Max(AttackData->MaximumLandingRadius, MinimumRadius);
	const float Radius = FMath::Sqrt(FMath::Lerp(
		FMath::Square(MinimumRadius),
		FMath::Square(MaximumRadius),
		FMath::FRand()));
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const FVector LocalScatterOffset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
	FVector LandingLocation = LandingCenter
		+ SourceTransform.TransformVectorNoScale(LocalScatterOffset);

	if (AttackData->bProjectLandingToGround)
	{
		if (UWorld* World = SourceEnemy->GetWorld())
		{
			const FVector TraceStart = LandingLocation
				+ FVector::UpVector * AttackData->GroundTraceStartOffset;
			const FVector TraceEnd = LandingLocation
				- FVector::UpVector * AttackData->GroundTraceDistance;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossSphereScatterGround), false, SourceEnemy.Get());
			QueryParams.AddIgnoredActor(SourceEnemy);

			FHitResult GroundHit;
			if (World->LineTraceSingleByChannel(
				GroundHit,
				TraceStart,
				TraceEnd,
				AttackData->GroundTraceChannel,
				QueryParams))
			{
				LandingLocation = GroundHit.ImpactPoint
					+ GroundHit.ImpactNormal * AttackData->GroundSurfaceOffset;
			}
		}
	}

	return LandingLocation;
}

bool UGA_BossSphereScatter::ResolveLaunchVelocity(
	const FVector& SpawnLocation,
	const FVector& LandingLocation,
	FVector& OutLaunchVelocity) const
{
	const UWorld* World = GetWorld();
	if (!World || !AttackData)
	{
		return false;
	}

	const float EffectiveGravityZ = World->GetGravityZ() * AttackData->ProjectileGravityScale;
	return UGameplayStatics::SuggestProjectileVelocity_CustomArc(
		this,
		OutLaunchVelocity,
		SpawnLocation,
		LandingLocation,
		EffectiveGravityZ,
		FMath::Clamp(AttackData->ArcParam, 0.05f, 0.95f));
}

void UGA_BossSphereScatter::FinishSpawning()
{
	if (bEndingAbility || !AttackData)
	{
		return;
	}
	if (SpawnedSphereCount <= 0)
	{
		UE_LOG(LogLastFPSBossSphereScatter, Error,
			TEXT("보스 구체 분산 공격 실패: Source=%s, 원인=생성에 성공한 구체가 없습니다."),
			*GetNameSafe(SourceEnemy));
		FinishCurrentAbility(true);
		return;
	}

	if (AttackData->RecoveryDuration <= 0.f)
	{
		FinishRecovery();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RecoveryTimerHandle,
			this,
			&UGA_BossSphereScatter::FinishRecovery,
			AttackData->RecoveryDuration,
			false);
	}
	else
	{
		FinishCurrentAbility(true);
	}
}

void UGA_BossSphereScatter::FinishRecovery()
{
	FinishCurrentAbility(false);
}

void UGA_BossSphereScatter::FinishCurrentAbility(const bool bWasCancelled)
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
