#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LastFPSScreenTypes.generated.h"

class ULastFPSActivatableWidget;
class UTexture2D;

/**
 * 화면 1개의 정의 — ScreenRegistry에 태그별로 등록되는 행.
 * 콘텐츠 추가 = 여기 한 행 채우기 (위젯 BP + 레이어 + 표시명).
 */
USTRUCT(BlueprintType)
struct FLastFPSScreenDef
{
	GENERATED_BODY()

	/** 표시할 Activatable 위젯 (지연 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	TSoftClassPtr<ULastFPSActivatableWidget> WidgetClass;

	/** push 대상 레이어. 미지정 시 UI.Layer.Menu로 폴백 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen", meta=(Categories="UI.Layer"))
	FGameplayTag LayerTag;

	/** 타이틀바 / 허브 탭 표시명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	FText DisplayName;

	/** 허브 탭/버튼 아이콘 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 허브 ESC 탭 메뉴에 자동 노출할지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	bool bShowInHubMenu = true;
};
