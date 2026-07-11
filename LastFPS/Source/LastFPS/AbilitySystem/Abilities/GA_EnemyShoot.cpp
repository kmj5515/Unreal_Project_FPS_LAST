#include "AbilitySystem/Abilities/GA_EnemyShoot.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Utility/LastFPSTags.h"

UGA_EnemyShoot::UGA_EnemyShoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// AI 는 서버에서만 도므로 예측 없이 서버 권위로 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 이 태그로 BTTask_EnemyAttack 이 발동한다(AIProfile.AttackAbilityTag 와 일치해야 함).
	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Shoot);
	SetAssetTags(Tags);
}

void UGA_EnemyShoot::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSCharacterBase* Self = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Self || !Self->IsAlive() || !ProjectileData || !ProjectileData->ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 발사체 스폰은 서버 권위에서만(ServerOnly 라 항상 서버지만 방어적으로 확인).
	if (GetWorld() && Self->HasAuthority())
	{
		SpawnProjectileAtTarget(Self);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_EnemyShoot::SpawnProjectileAtTarget(ALastFPSCharacterBase* Self) const
{
	UWorld* World = GetWorld();
	if (!World || !Self || !ProjectileData || !ProjectileData->ProjectileClass)
	{
		return;
	}

	// 타깃: BT 가 SetFocus 한 AIController 의 FocusActor.
	AActor* Target = nullptr;
	if (const AAIController* AICon = Cast<AAIController>(Self->GetController()))
	{
		Target = AICon->GetFocusActor();
	}

	// 스폰 위치: 소켓이 있으면 소켓, 없으면 액터 + 총구 높이.
	FVector SpawnLocation = Self->GetActorLocation() + FVector(0.f, 0.f, MuzzleHeight);
	if (const USkeletalMeshComponent* Mesh = Self->GetMesh())
	{
		if (!ProjectileData->SpawnSocketName.IsNone() && Mesh->DoesSocketExist(ProjectileData->SpawnSocketName))
		{
			SpawnLocation = Mesh->GetSocketLocation(ProjectileData->SpawnSocketName);
		}
	}

	// 조준: 타깃 몸통을 향해. 타깃이 없으면 액터 전방.
	FVector AimDirection = Self->GetActorForwardVector();
	if (Target)
	{
		const FVector AimPoint = Target->GetActorLocation() + FVector(0.f, 0.f, TargetAimHeight);
		const FVector ToTarget = (AimPoint - SpawnLocation).GetSafeNormal();
		if (!ToTarget.IsNearlyZero())
		{
			AimDirection = ToTarget;
		}
	}

	const FRotator SpawnRotation = AimDirection.Rotation();
	SpawnLocation += SpawnRotation.RotateVector(ProjectileData->SpawnLocationOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Self;
	SpawnParams.Instigator = Cast<APawn>(Self);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALastFPSProjectile* Projectile = World->SpawnActor<ALastFPSProjectile>(
		ProjectileData->ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeGameplayProjectile(
		Self,
		ProjectileData->ImpactRules,
		ProjectileData->EffectsOnHit,
		ProjectileData->VisualData);

	if (Projectile->ProjectileMovement)
	{
		Projectile->ProjectileMovement->Velocity = AimDirection * ProjectileData->ProjectileSpeed;
	}
}
