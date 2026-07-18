#include "AbilitySystem/Abilities/GA_EnemyShoot.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Character/LastFPSCharacterBase.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Engine/World.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Projectiles/LastFPSProjectileLaunchUtility.h"
#include "Utility/LastFPSTags.h"

UGA_EnemyShoot::UGA_EnemyShoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// AI 는 서버에서만 도므로 예측 없이 서버 권위로 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// BTTask_EnemyAttack에 같은 태그를 지정해 이 어빌리티를 활성화한다.
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

	FVector AimTarget = Self->GetActorLocation()
		+ Self->GetActorForwardVector() * ProjectileData->AimTraceRange;
	if (Target)
	{
		AimTarget = Target->GetActorLocation() + FVector(0.f, 0.f, TargetAimHeight);
	}

	FLastFPSProjectileLaunchRequest LaunchRequest;
	LaunchRequest.SourceActor = Self;
	LaunchRequest.ProjectileData = ProjectileData;
	LaunchRequest.AimTarget = AimTarget;
	LaunchRequest.FallbackAimDirection = Self->GetActorForwardVector();
	LaunchRequest.FallbackMuzzleHeight = MuzzleHeight;
	LaunchRequest.bUseEquippedWeapon = true;
	LastFPSProjectileLaunch::SpawnProjectile(LaunchRequest);
}
