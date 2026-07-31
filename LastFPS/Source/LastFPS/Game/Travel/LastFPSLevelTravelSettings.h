#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSLevelTravelSettings.generated.h"

enum class ELastFPSTravelDestination : uint8;

/** 프로젝트 공통 로컬 화면 경로를 Primary Asset ID로 관리한다. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LastFPS Level Travel"))
class LASTFPS_API ULastFPSLevelTravelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULastFPSLevelTravelSettings();

	const FPrimaryAssetId& GetMapId(ELastFPSTravelDestination Destination) const;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId MainMenuMapId;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId CharacterSelectMapId;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId HubMapId;
};
