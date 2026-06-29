#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UI/Framework/LastFPSScreenTypes.h"
#include "LastFPSScreenRegistry.generated.h"

/**
 * 화면 태그 → 정의 매핑 DataAsset.
 * UISettings(프로젝트 세팅)가 이 에셋 하나를 가리키고, UIManagerSubsystem이 조회한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSScreenRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** UI.Screen.* 태그 → 화면 정의 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screens", meta=(ForceInlineRow, Categories="UI.Screen"))
	TMap<FGameplayTag, FLastFPSScreenDef> Screens;

	/** 없으면 nullptr */
	const FLastFPSScreenDef* FindScreen(const FGameplayTag& ScreenTag) const;
};
