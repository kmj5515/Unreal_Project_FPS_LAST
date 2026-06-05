#pragma once

#include "CommonButtonBase.h"
#include "LastFPSButtonBase.generated.h"

class UTextBlock;

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

	/** WBP에 동일 이름으로 TextBlock을 두면 자동 바인딩 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock;

	/** 인스턴스별 라벨. 부모 WBP의 각 버튼 Details에서 입력. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Button")
	FText ButtonText;
};
