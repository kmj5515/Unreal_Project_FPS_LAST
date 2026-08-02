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

	/**
	 * 지정한 껍데기가 품는 탭 화면 태그를 TabOrder 순으로 돌려준다.
	 *
	 * 껍데기가 탭 목록을 따로 저작하면 버튼 배치와 두 곳을 손으로 맞춰야 하고 어긋나도 조용히 넘어간다.
	 * 목록의 단일 출처를 레지스트리로 두어 저작 지점을 하나로 만든다.
	 */
	void GetTabScreensForHost(const FGameplayTag& HostScreenTag, TArray<FGameplayTag>& OutTabTags) const;
};
