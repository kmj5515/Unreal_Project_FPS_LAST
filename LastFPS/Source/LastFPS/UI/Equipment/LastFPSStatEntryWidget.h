#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Theme/LastFPSUIThemeReceiver.h"
#include "LastFPSStatEntryWidget.generated.h"

class UTextBlock;

/**
 * WBP_StatEntry 의 Parent — 스탯 표시 1행(이름 + 값).
 *
 * 요약 패널이 스탯 개수만큼 멀티라인 문자열을 조립하면 행 단위 스타일·정렬·강조를 줄 수 없고,
 * 값 서식이 바뀔 때마다 표시 코드를 함께 고쳐야 한다. 행 하나를 위젯으로 분리해 표시 책임을
 * 여기로 모으고, 목록을 소유한 쪽은 "어떤 행을 몇 개 보여줄지"만 결정하게 한다.
 *
 * 값의 의미(스탯 종류·계산)는 알지 않는다. 이미 서식이 끝난 FText 두 개와 부호만 받는다.
 *
 * Designer 바인딩(선택): TB_StatName / TB_StatValue
 */
UCLASS()
class LASTFPS_API ULastFPSStatEntryWidget : public UUserWidget, public ILastFPSUIThemeReceiver
{
	GENERATED_BODY()

public:
	/**
	 * 한 행의 표시 내용을 채운다.
	 * @param InSign 값의 부호(-1/0/1). 값 서식이 호출부마다 달라도 색상 규칙은 한 곳에 두려고 따로 받는다.
	 */
	void SetEntry(const FText& InName, const FText& InValue, int32 InSign);

	virtual void ApplyUITheme(const ULastFPSUIThemeAsset& Theme) override;

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatName;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TB_StatValue;

private:
	/** 마지막으로 표시한 부호. 테마가 늦게 적용돼도 색을 다시 계산할 수 있게 들고 있는다. */
	int32 CachedSign = 0;

	/**
	 * 표시색 — 테마가 적용되면 덮어쓴다.
	 * 테마 지정이 누락돼도 값이 배경에 묻히지 않도록 최소한의 기본값을 둔다.
	 */
	FLinearColor PositiveValueColor = FLinearColor(0.25f, 0.90f, 0.35f, 1.f);
	FLinearColor NegativeValueColor = FLinearColor(0.90f, 0.30f, 0.30f, 1.f);
	FLinearColor NeutralValueColor = FLinearColor::White;

	void RefreshValueColor();
};
