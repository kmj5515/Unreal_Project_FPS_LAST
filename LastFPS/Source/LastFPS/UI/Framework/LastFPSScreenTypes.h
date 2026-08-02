#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LastFPSScreenTypes.generated.h"

class ULastFPSActivatableWidget;
class UTexture2D;

/**
 * 화면을 어떻게 띄우는지.
 *
 * 같은 태그가 상황에 따라 레이어 push 가 되기도 하고 탭 전환이 되기도 하면, 어느 쪽으로 열릴지가
 * "그 순간 껍데기가 떠 있는가"라는 런타임 우연에 좌우된다. 표시 방식을 데이터로 못박아 결정적으로 만든다.
 */
UENUM(BlueprintType)
enum class ELastFPSScreenPresentation : uint8
{
	/** 레이어 스택에 push 한다 (메인메뉴·캐릭터선택·허브메뉴 등 독립 화면) */
	Layer,
	/** 껍데기 화면 안의 스위처에서 전환한다 (허브 탭 콘텐츠) */
	ShellTab,
};

/**
 * 화면 1개의 정의 — ScreenRegistry에 태그별로 등록되는 행.
 * 콘텐츠 추가 = 여기 한 행 채우기 (위젯 BP + 표시 방식 + 표시명).
 */
USTRUCT(BlueprintType)
struct FLastFPSScreenDef
{
	GENERATED_BODY()

	/** 표시할 Activatable 위젯 (지연 로드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	TSoftClassPtr<ULastFPSActivatableWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen")
	ELastFPSScreenPresentation Presentation = ELastFPSScreenPresentation::Layer;

	/**
	 * ShellTab 일 때 이 화면을 품는 껍데기(예: UI.Screen.HubMenu).
	 * 껍데기가 닫혀 있으면 이것을 먼저 열고 탭으로 이어간다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen",
		meta=(Categories="UI.Screen",
			EditCondition="Presentation == ELastFPSScreenPresentation::ShellTab",
			EditConditionHides))
	FGameplayTag HostScreenTag;

	/**
	 * 껍데기 안에서의 탭 순서. 껍데기는 이 값으로 정렬해 탭 목록을 만든다.
	 * 버튼 배치 순서와 이 값만 맞추면 되고, 태그 배열을 따로 저작할 필요가 없다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen",
		meta=(EditCondition="Presentation == ELastFPSScreenPresentation::ShellTab",
			EditConditionHides))
	int32 TabOrder = 0;

	/** push 대상 레이어. 미지정 시 UI.Layer.Menu로 폴백. ShellTab 은 사용하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen",
		meta=(Categories="UI.Layer",
			EditCondition="Presentation == ELastFPSScreenPresentation::Layer",
			EditConditionHides))
	FGameplayTag LayerTag;

	/**
	 * 이 화면을 볼 때 프리뷰 무대가 켤 시점(UI.Preview.View.*).
	 *
	 * 화면마다 뒤에 무엇을 보여줄지가 다르다(장비는 캐릭터, 지도는 아무것도 아님).
	 * 껍데기가 화면 태그를 나열해 분기하면 화면이 늘 때마다 껍데기를 고쳐야 하므로,
	 * 각 화면이 자기가 원하는 시점을 데이터로 선언하고 껍데기는 그대로 전달만 한다.
	 *
	 * 비워 두면 시점을 바꾸지 않는다(직전 화면의 시점을 유지).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screen", meta=(Categories="UI.Preview.View"))
	FGameplayTag PreviewViewTag;

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
