#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSPopupCatalog.generated.h"

class ULastFPSModalDialogBase;

UENUM(BlueprintType)
enum class ELastFPSPopupLoadTiming : uint8
{
	LocalPlayerInitialize,
	DestinationEnter,
	FirstRequest,
};

UENUM(BlueprintType)
enum class ELastFPSPopupAssetReleaseTiming : uint8
{
	LevelTransitionStart,
	DestinationExit,
	LocalPlayerDeinitialize,
};

UENUM(BlueprintType)
enum class ELastFPSPopupInstanceCloseTiming : uint8
{
	LevelTransitionStart,
	DestinationExit,
	Manual,
};

/** 팝업 하나의 에셋과 화면 인스턴스 수명을 정의한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSPopupLifecyclePolicy
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lifecycle")
	ELastFPSPopupLoadTiming LoadTiming =
		ELastFPSPopupLoadTiming::FirstRequest;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lifecycle")
	ELastFPSPopupAssetReleaseTiming AssetReleaseTiming =
		ELastFPSPopupAssetReleaseTiming::DestinationExit;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lifecycle")
	ELastFPSPopupInstanceCloseTiming InstanceCloseTiming =
		ELastFPSPopupInstanceCloseTiming::LevelTransitionStart;

	/**
	 * 비어 있으면 모든 목적지에서 사용할 수 있다.
	 * 목적지 진입 로드와 목적지 이탈 해제 여부를 같은 계약으로 판정한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lifecycle")
	FGameplayTagQuery ContextQuery;
};

/** 단일 PopupCatalog 안에서 팝업 하나를 설명하는 안정적인 데이터 계약이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSPopupEntry
{
	GENERATED_BODY()

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Popup",
		meta=(Categories="UI.Popup"))
	FGameplayTag PopupTag;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category="Popup",
		meta=(AssetBundles="UI"))
	TSoftClassPtr<ULastFPSModalDialogBase> WidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Popup")
	FLastFPSPopupLifecyclePolicy Lifecycle;
};

/** 모든 팝업 목록을 보관하며 실제 로드 단위는 각 Entry의 수명 정책으로 결정한다. */
UCLASS(BlueprintType, Const)
class LASTFPS_API ULastFPSPopupCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Popup")
	TArray<FLastFPSPopupEntry> Popups;
};
