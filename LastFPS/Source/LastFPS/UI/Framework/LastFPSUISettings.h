#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSUISettings.generated.h"

class ULastFPSScreenRegistry;

/**
 * 아웃게임 UI 설정 - Project Settings > MWTool > UI.
 * 화면 라우팅에 사용할 ScreenRegistry를 지정한다.
 */
UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS UI"))
class LASTFPS_API ULastFPSUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif

	/** OpenScreen이 조회할 화면 레지스트리 */
	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftObjectPtr<ULastFPSScreenRegistry> ScreenRegistry;
};
