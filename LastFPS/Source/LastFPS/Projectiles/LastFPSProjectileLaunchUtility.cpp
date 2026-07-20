#include "Projectiles/LastFPSProjectileLaunchUtility.h"

#include "Character/Components/WeaponComponent.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectiles/LastFPSProjectile.h"

namespace
{
	FVector ResolveLaunchLocation(
		AActor* SourceActor,
		const ULastFPSAbilityProjectileData* ProjectileData,
		const float FallbackMuzzleHeight,
		const bool bUseEquippedWeapon)
	{
		if (bUseEquippedWeapon)
		{
			if (const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(SourceActor))
			{
				if (const UWeaponComponent* WeaponComponent = WeaponUser->GetWeaponComponent();
					WeaponComponent && WeaponComponent->CanFire())
				{
					return WeaponComponent->GetMuzzleTransform().GetLocation();
				}
			}
		}

		if (const ACharacter* Character = Cast<ACharacter>(SourceActor))
		{
			if (const USkeletalMeshComponent* Mesh = Character->GetMesh();
				Mesh && !ProjectileData->SpawnSocketName.IsNone()
				&& Mesh->DoesSocketExist(ProjectileData->SpawnSocketName))
			{
				return Mesh->GetSocketLocation(ProjectileData->SpawnSocketName);
			}
		}

		return SourceActor->GetActorLocation() + FVector(0.f, 0.f, FallbackMuzzleHeight);
	}
}

ALastFPSProjectile* LastFPSProjectileLaunch::SpawnProjectile(const FLastFPSProjectileLaunchRequest& Request)
{
	AActor* SourceActor = Request.SourceActor;
	const ULastFPSAbilityProjectileData* ProjectileData = Request.ProjectileData;
	UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
	if (!World || !SourceActor || !SourceActor->HasAuthority() || !ProjectileData || !ProjectileData->ProjectileClass)
	{
		return nullptr;
	}

	FVector SpawnLocation = Request.bOverrideSpawnLocation
		? Request.SpawnLocationOverride
		: ResolveLaunchLocation(
			SourceActor,
			ProjectileData,
			Request.FallbackMuzzleHeight,
			Request.bUseEquippedWeapon);
	FVector AimDirection = Request.bOverrideLaunchVelocity
		? Request.LaunchVelocityOverride.GetSafeNormal()
		: (Request.AimTarget - SpawnLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = Request.FallbackAimDirection.GetSafeNormal();
	}
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = SourceActor->GetActorForwardVector();
	}

	if (Request.bApplyProjectileDataSpawnOffset)
	{
		SpawnLocation += AimDirection.Rotation().RotateVector(ProjectileData->SpawnLocationOffset);
		if (!Request.bOverrideLaunchVelocity)
		{
			const FVector AdjustedAimDirection = (Request.AimTarget - SpawnLocation).GetSafeNormal();
			if (!AdjustedAimDirection.IsNearlyZero())
			{
				AimDirection = AdjustedAimDirection;
			}
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = Cast<APawn>(SourceActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<ALastFPSProjectile> ProjectileClass = ProjectileData->ProjectileClass;
	if (Request.bUseEquippedWeapon)
	{
		if (const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(SourceActor))
		{
			if (const UWeaponComponent* WeaponComponent = WeaponUser->GetWeaponComponent();
				WeaponComponent && WeaponComponent->ProjectileClass)
			{
				ProjectileClass = WeaponComponent->ProjectileClass;
			}
		}
	}

	ALastFPSProjectile* Projectile = World->SpawnActor<ALastFPSProjectile>(
		ProjectileClass,
		SpawnLocation,
		AimDirection.Rotation(),
		SpawnParams);
	if (!Projectile)
	{
		return nullptr;
	}

	Projectile->InitializeGameplayProjectile(
		SourceActor,
		ProjectileData->ImpactRules,
		ProjectileData->EffectsOnHit,
		ProjectileData->VisualData,
		Request.BaseDamageOverride,
		&ProjectileData->CollisionSettings);

	if (Projectile->ProjectileMovement)
	{
		if (Request.bOverrideGravityScale)
		{
			Projectile->ProjectileMovement->ProjectileGravityScale = Request.GravityScaleOverride;
		}
		Projectile->ProjectileMovement->Velocity = Request.bOverrideLaunchVelocity
			? Request.LaunchVelocityOverride
			: AimDirection * ProjectileData->ProjectileSpeed;
	}
	if (Request.LifeSpanOverride > 0.f)
	{
		Projectile->SetLifeSpan(Request.LifeSpanOverride);
	}

	return Projectile;
}
