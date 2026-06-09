#pragma once

#include "GameFramework/GameUserSettings.h"
#include "LastFPSGameUserSettings.generated.h"

/**
 * 프로젝트 커스텀 UserSettings — 그래픽·사운드·입력 감도를 한 곳에서 저장.
 * DefaultEngine.ini [/Script/Engine.Engine] GameUserSettingsClassName 으로 등록됨.
 */
UCLASS()
class LASTFPS_API ULastFPSGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static ULastFPSGameUserSettings* Get();

	UPROPERTY(Config, BlueprintReadWrite, Category="Audio")
	float MasterVolume = 1.f;

	UPROPERTY(Config, BlueprintReadWrite, Category="Audio")
	float MusicVolume = 1.f;

	UPROPERTY(Config, BlueprintReadWrite, Category="Audio")
	float SFXVolume = 1.f;

	/** 0.0 ~ 1.0 정규화. 실제 스케일 적용은 WBP_Settings BP (Enhanced Input Modifier 연결) */
	UPROPERTY(Config, BlueprintReadWrite, Category="Input")
	float MouseSensitivity = 0.5f;
};
