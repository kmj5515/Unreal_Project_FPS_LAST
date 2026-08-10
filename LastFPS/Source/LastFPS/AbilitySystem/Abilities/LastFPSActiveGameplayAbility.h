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

	/** 아바타가 확정된 시점에 쿨다운 신원을 한 번 해석해 캐시한다. */
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
	 * GetCooldownTags 가 포인터를 돌려주므로 주소가 안정적인 인스턴스 저장이 필요하다.
	 * 모든 어빌리티가 InstancedPerActor 라 인스턴스 멤버로 충분하다.
	 */
	FGameplayTagContainer CachedCooldownTags;

	float CachedCooldownSeconds = 0.f;
};
