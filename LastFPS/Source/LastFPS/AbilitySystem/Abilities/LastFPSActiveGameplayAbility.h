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

	/** 로컬 소유자에게 시전 시작음을 한 번 재생해 예측 실행과 서버 실행의 중복을 피한다. */
	void PlayActivationSound() const;

	/** Sound Cue에서 감쇠와 음색 변형을 구성하고 어빌리티에는 재생할 에셋만 연결한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio")
	TObjectPtr<USoundBase> ActivationSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio", meta=(ClampMin="0.0"))
	float ActivationSoundVolumeMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Audio", meta=(ClampMin="0.01"))
	float ActivationSoundPitchMultiplier = 1.f;
};
