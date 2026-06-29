#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSContentScreenWidget.generated.h"

class UTextBlock;
class ULastFPSButtonBase;

/**
 * 풀스크린 콘텐츠 화면 공용 베이스 (인벤토리 / 임무 / 상점 / 설정 ...).
 * 공통 크롬: 타이틀 텍스트 + 닫기 버튼. 콘텐츠는 파생 WBP가 채운다.
 * 닫기 / Back → 위젯 비활성화(레이어에서 pop).
 */
UCLASS()
class LASTFPS_API ULastFPSContentScreenWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	/** 타이틀바 텍스트 (BP에서 화면별 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	FText ScreenTitle;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Close;

private:
	UFUNCTION()
	void HandleCloseClicked();
};
