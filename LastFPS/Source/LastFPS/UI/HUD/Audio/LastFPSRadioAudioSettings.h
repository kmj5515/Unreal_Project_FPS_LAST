#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSRadioAudioSettings.generated.h"

class USoundBase;

/**
 * 모든 HUD 무전 음성에 공통으로 적용할 오디오 프로필이다.
 * 대사별 데이터는 DT_RadioTransmission이 소유하고, 매체 표현만 이 설정에서 일관되게 관리한다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LastFPS Radio Audio"))
class LASTFPS_API ULastFPSRadioAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const ULastFPSRadioAudioSettings* Get()
	{
		return GetDefault<ULastFPSRadioAudioSettings>();
	}

	virtual FName GetCategoryName() const override
	{
		return FName(TEXT("Game"));
	}

	UPROPERTY(Config, EditAnywhere, Category="Voice")
	bool bEnableRadioEffect = true;

	UPROPERTY(Config, EditAnywhere, Category="Voice", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float VoiceVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Filter", meta=(ClampMin="20.0", ClampMax="20000.0", Units="Hz"))
	float HighPassCutoffFrequency = 300.0f;

	UPROPERTY(Config, EditAnywhere, Category="Filter", meta=(ClampMin="20.0", ClampMax="20000.0", Units="Hz"))
	float LowPassCutoffFrequency = 3400.0f;

	UPROPERTY(Config, EditAnywhere, Category="Filter", meta=(ClampMin="0.5", ClampMax="10.0"))
	float FilterQ = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category="Distortion")
	bool bEnableDistortion = true;

	UPROPERTY(Config, EditAnywhere, Category="Distortion", meta=(ClampMin="0.0", ClampMax="500.0"))
	float DistortionAmount = 2.0f;

	UPROPERTY(Config, EditAnywhere, Category="Distortion", meta=(ClampMin="-60.0", ClampMax="20.0"))
	float DistortionOutputGainDb = -1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Compressor", meta=(ClampMin="-60.0", ClampMax="0.0"))
	float CompressorThresholdDb = -18.0f;

	UPROPERTY(Config, EditAnywhere, Category="Compressor", meta=(ClampMin="1.0", ClampMax="20.0"))
	float CompressorRatio = 3.5f;

	UPROPERTY(Config, EditAnywhere, Category="Compressor", meta=(ClampMin="1.0", ClampMax="300.0", Units="ms"))
	float CompressorAttackTimeMsec = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category="Compressor", meta=(ClampMin="20.0", ClampMax="5000.0", Units="ms"))
	float CompressorReleaseTimeMsec = 80.0f;

	UPROPERTY(Config, EditAnywhere, Category="Compressor", meta=(ClampMin="0.0", ClampMax="20.0"))
	float CompressorOutputGainDb = 2.0f;

	UPROPERTY(Config, EditAnywhere, Category="Bit Crusher")
	bool bEnableBitCrusher = false;

	UPROPERTY(Config, EditAnywhere, Category="Bit Crusher", meta=(ClampMin="500.0", ClampMax="96000.0", Units="Hz"))
	float BitCrusherSampleRate = 14000.0f;

	UPROPERTY(Config, EditAnywhere, Category="Bit Crusher", meta=(ClampMin="1.0", ClampMax="24.0"))
	float BitCrusherBitDepth = 11.0f;

	/** 지정하면 각 무전 대사 직전에 로컬 2D 사운드로 재생한다. */
	UPROPERTY(Config, EditAnywhere, Category="Transmission Signal", meta=(AllowedClasses="/Script/Engine.SoundBase"))
	TSoftObjectPtr<USoundBase> TransmissionOpenSound;

	/** 지정하면 각 무전 대사 직후에 로컬 2D 사운드로 재생한다. */
	UPROPERTY(Config, EditAnywhere, Category="Transmission Signal", meta=(AllowedClasses="/Script/Engine.SoundBase"))
	TSoftObjectPtr<USoundBase> TransmissionCloseSound;

	UPROPERTY(Config, EditAnywhere, Category="Transmission Signal", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float TransmissionSignalVolume = 0.8f;
};
