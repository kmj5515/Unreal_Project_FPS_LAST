#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSTravelTypes.generated.h"

/** 아웃게임 맵 이동 대상 */
UENUM(BlueprintType)
enum class ELastFPSTravelDestination : uint8
{
	MainMenu,
	CharacterSelect,
	Hub,
};

/** 공통 이동 버튼이 요청할 이동 방식이다. */
UENUM(BlueprintType)
enum class ELastFPSTravelEntryType : uint8
{
	None, 
	LocalDestination,
	QuickPlayBattle,
};

/** UI가 이동 구현을 알지 않고 전달하는 안정적인 요청 데이터다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSTravelEntryRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel")
	ELastFPSTravelEntryType Type =
		ELastFPSTravelEntryType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel",
		meta=(
			EditCondition="Type == ELastFPSTravelEntryType::LocalDestination",
			EditConditionHides))
	ELastFPSTravelDestination Destination =
		ELastFPSTravelDestination::MainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Travel"
		,meta=(AllowedTypes="BattleDefinition",
		EditCondition="Type == ELastFPSTravelEntryType::QuickPlayBattle",
			EditConditionHides))
	FPrimaryAssetId BattleDefinitionId;
};

/** 이동 이후 로딩 완료로 판단할 로컬 플레이어 준비 수준 */
UENUM(BlueprintType)
enum class ELastFPSTravelReadiness : uint8
{
	LevelAndController,
	LevelControllerAndPawn,
};

/** 전역 이동 요청의 현재 처리 단계 */
UENUM(BlueprintType)
enum class ELastFPSTravelState : uint8
{
	Idle,
	LoadingDefinition,
	SearchingSession,
	JoiningSession,
	HostingSession,
	Travelling,
};

/** 이동 요청을 즉시 수락하지 못한 이유 */
UENUM(BlueprintType)
enum class ELastFPSTravelRequestResult : uint8
{
	Accepted,
	Busy,
	InvalidAssetId,
	AssetNotFound,
	InvalidDefinition,
	InvalidWorld,
	InvalidPlayer,
	SessionUnavailable,
	MissingRequiredWeapon,
};
