#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/LastFPSGameplayAbility.h"
#include "LastFPSActiveGameplayAbility.generated.h"

class USoundBase;

UCLASS(Abstract)
class LASTFPS_API ULastFPSActiveGameplayAbility : public ULastFPSGameplayAbility
{
	GENERATED_BODY()

public:
	ULastFPSActiveGameplayAbility();

protected:
	virtual void ApplyCooldown(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 아바타가 바뀌면 캐시를 버린다. 캐릭터가 달라지면 쿨다운 신원도 달라지기 때문이다. */
	virtual void OnAvatarSet(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

	/**
	 * 쿨다운 판정에 쓸 태그를 데이터에서 캐시한 값으로 돌려준다.
	 *
	 * 엔진 기본 구현은 쿨다운 GE 클래스(CDO)가 부여하는 태그를 읽는다. 그 방식은 슬롯마다 GE 클래스를
	 * 따로 두게 만들고, 스킬 테이블이 이미 소유한 CooldownTag 와 태그가 이중으로 존재하게 된다.
	 * 여기서 데이터 쪽을 단일 권위로 삼는다.
	 */
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	/** 로컬 소유자에게 시전 시작음을 한 번 재생해 예측 실행과 서버 실행의 중복을 피한다. */
	void PlayActivationSound() const;

	/** Sound Cue에서 감쇠와 음색 변형을 구성하고 어빌리티에는 재생할 에셋만 연결한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio")
	TObjectPtr<USoundBase> ActivationSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio", meta=(ClampMin="0.0"))
	float ActivationSoundVolumeMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio", meta=(ClampMin="0.01"))
	float ActivationSoundPitchMultiplier = 1.f;

private:
	/**
	 * 아직 해석하지 못했으면 지금 해석해 캐시한다.
	 *
	 * 아바타 확정 시점에 한 번만 해석하면, 캐릭터 정의 복제가 어빌리티 부여보다 늦게 도착하는
	 * 클라이언트에서 캐시가 영구히 빈 채로 남는다. 그래서 필요한 시점마다 다시 시도한다.
	 */
	void EnsureCooldownIdentity() const;

	/**
	 * GetCooldownTags 가 포인터를 돌려주므로 주소가 안정적인 인스턴스 저장이 필요하다.
	 * 모든 어빌리티가 InstancedPerActor 라 인스턴스 멤버로 충분하다.
	 * const 조회 경로에서 늦게 채워지므로 mutable 이다.
	 */
	mutable FGameplayTagContainer CachedCooldownTags;

	mutable float CachedCooldownSeconds = 0.f;
};
