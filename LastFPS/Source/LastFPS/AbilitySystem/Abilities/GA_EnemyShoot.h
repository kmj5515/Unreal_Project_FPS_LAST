#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "GA_EnemyShoot.generated.h"

class ALastFPSCharacterBase;
class ULastFPSAbilityProjectileData;

/**
 * 적 AI 전용 원거리 발사 어빌리티.
 *
 * GA_BasicShoot / GA_Projectile 은 Hero(플레이어) 전용(카메라 조준·WeaponComponent·예측)이라 적에게 못 쓴다.
 * 이 어빌리티는 Hero 의존 없이 ALastFPSCharacterBase 기준으로 동작한다:
 *   - 조준: AIController 의 FocusActor(=BT 가 SetFocus 한 타깃)를 향해 발사.
 *   - 스폰: ProjectileData 로 발사체를 스폰하고 데미지/임팩트 규칙을 초기화(서버 권위).
 * 발동은 BTTask_EnemyAttack에 지정된 Ability.Enemy.Shoot 태그로 수행한다.
 */
UCLASS()
class LASTFPS_API UGA_EnemyShoot : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyShoot();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** 발사체/데미지/비주얼 데이터(적 무기 정의). 없으면 발동 실패. */
	UPROPERTY(EditDefaultsOnly, Category="Enemy|Shoot")
	TObjectPtr<ULastFPSAbilityProjectileData> ProjectileData;

	/** 소켓이 없을 때 사용할 총구 높이(cm, 액터 기준). */
	UPROPERTY(EditDefaultsOnly, Category="Enemy|Shoot", meta=(ClampMin="0.0"))
	float MuzzleHeight = 60.f;

	/** 타깃 조준점 높이 오프셋(cm). 발밑이 아니라 몸통을 겨냥. */
	UPROPERTY(EditDefaultsOnly, Category="Enemy|Shoot")
	float TargetAimHeight = 50.f;

private:
	/** 서버: FocusActor 방향으로 발사체 1발 스폰. */
	void SpawnProjectileAtTarget(ALastFPSCharacterBase* Self) const;
};
