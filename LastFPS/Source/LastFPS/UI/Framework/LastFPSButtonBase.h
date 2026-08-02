#pragma once

#include "CommonButtonBase.h"
#include "LastFPSButtonBase.generated.h"

class UCommonTextBlock;

/**
 * 프로젝트 공통 버튼 베이스.
 * 모든 UI 버튼은 이 클래스를 상속한 WBP(WBP_LastFPSButton 등)를 사용한다.
 * CommonUI 입력 라우팅(키보드/패드 포커스 내비게이션)을 적용하기 위한 단일 진입점.
 *
 * WBP에 TextBlock을 'TextBlock' 이름으로 배치하면 라벨이 자동 연결된다.
 * 인스턴스 Details의 ButtonText 값이 PreConstruct에서 반영되어 에디터에서 바로 보인다.
 */
UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	/** 런타임에 라벨 변경 (예: 동적 텍스트). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Button")
	void SetButtonText(const FText& InText);

	UFUNCTION(BlueprintPure, Category="LastFPS|Button")
	FText GetButtonText() const { return ButtonText; }

protected:
	virtual void NativePreConstruct() override;

	/**
	 * 버튼 상태(Normal/Selected/Disabled)에 맞는 텍스트 스타일을 라벨에 옮긴다.
	 *
	 * CommonUI 는 스타일이 바뀌었다는 통보만 하고 적용은 버튼에 맡긴다. 이 배선을 파생 WBP 마다
	 * 넣으면 버튼 종류만큼 같은 그래프가 반복되므로 베이스에서 한 번만 처리한다.
	 */
	virtual void NativeOnCurrentTextStyleChanged() override;

	/** WBP에 동일 이름으로 TextBlock을 두면 자동 바인딩 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TextBlock;

	/** 인스턴스별 라벨. 부모 WBP의 각 버튼 Details에서 입력. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Button")
	FText ButtonText;
};
