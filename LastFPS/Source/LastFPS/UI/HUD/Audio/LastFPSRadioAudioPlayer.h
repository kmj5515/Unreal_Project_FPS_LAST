#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSRadioAudioPlayer.generated.h"

class UAudioComponent;
class USoundBase;
class USoundEffectSourcePreset;
class USoundEffectSourcePresetChain;

/**
 * HUD 무전의 재생과 매체 효과 구성을 전담한다.
 * 위젯은 표시 순서만 관리하고 구체적인 오디오 효과 구현에는 의존하지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSRadioAudioPlayer : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UObject* InWorldContext);
	void PlayTransmissionStart();
	void PlayVoice(const TSoftObjectPtr<USoundBase>& VoiceSound);
	void PlayTransmissionEnd();
	void Stop();

private:
	void BuildEffectChain();
	void PlaySignal(const TSoftObjectPtr<USoundBase>& SignalSound);

	TWeakObjectPtr<UObject> WorldContext;
	TWeakObjectPtr<UAudioComponent> ActiveVoiceAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundEffectSourcePresetChain> RadioEffectChain;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundEffectSourcePreset>> EffectPresets;
};
