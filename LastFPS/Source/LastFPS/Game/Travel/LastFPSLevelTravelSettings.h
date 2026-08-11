#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSLevelTravelSettings.generated.h"

enum class ELastFPSTravelDestination : uint8;
class UTexture2D;

/** 전투 목적지의 퀘스트 기반 접근 조건이다. 목적지별 예외를 UI 코드에 박지 않도록 설정으로 관리한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSTravelQuestRequirement
{
	GENERATED_BODY()

	/** 접근 조건을 적용할 전투 정의다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel", meta=(AllowedTypes="BattleDefinition"))
	FPrimaryAssetId BattleDefinitionId;

	/** 진행 중이어야 출격 가능한 퀘스트다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
	FName RequiredQuestId;

	/** 퀘스트 수령 완료 뒤에도 재도전을 허용할지 결정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
	bool bAllowReplayAfterClaimed = true;

	/** 잠긴 목적지 위에 표시할 UI 텍스처다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
	TSoftObjectPtr<UTexture2D> LockedIcon;
};

/** 프로젝트 공통 로컬 화면 경로를 Primary Asset ID로 관리한다. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LastFPS Level Travel"))
class LASTFPS_API ULastFPSLevelTravelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULastFPSLevelTravelSettings();

	const FPrimaryAssetId& GetMapId(ELastFPSTravelDestination Destination) const;
	const FLastFPSTravelQuestRequirement* FindQuestRequirement(
		const FPrimaryAssetId& BattleDefinitionId) const;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId MainMenuMapId;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId CharacterSelectMapId;

	UPROPERTY(Config, EditAnywhere, Category="Local Maps", meta=(AllowedTypes="Map"))
	FPrimaryAssetId HubMapId;

	/** 전투 목적지별 퀘스트 접근 조건이다. 배열 추가만으로 다른 목적지도 확장한다. */
	UPROPERTY(Config, EditAnywhere, Category="Battle Access")
	TArray<FLastFPSTravelQuestRequirement> BattleQuestRequirements;
};
