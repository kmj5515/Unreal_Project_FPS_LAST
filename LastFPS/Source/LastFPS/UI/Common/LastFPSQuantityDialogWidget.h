#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSQuantityDialogWidget.generated.h"

class ULastFPSButtonBase;
class UTextBlock;

/** 수량 선택 결과 — Quantity > 0: 확정(그 수량), 0: 취소 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSQuantityResult, int32, Quantity);

/**
 * 수량 선택 모달 — 상점에서 한 번에 여러 개 구매할 때 +/- 로 수량을 고른다.
 * ConfirmWidget 과 동일하게 모달 레이어에 push 되며, 결과는 OnQuantityResult 로 전달.
 *
 * Designer 바인딩(선택):
 *   Text_Title / Text_Body — 제목 / 아이템 이름 (ModalDialogBase 상속)
 *   Button_Plus / Button_Minus — 수량 증감
 *   TB_Quantity   — 현재 수량
 *   TB_TotalPrice — 단가 × 수량 합계
 *   TB_MaxInfo    — "최대 N개 구매 가능" 안내
 *   Button_Confirm / Button_Cancel
 */
UCLASS()
class LASTFPS_API ULastFPSQuantityDialogWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	/** 제목 / 아이템명 / 단가 / 최대 구매 수량으로 초기화 (수량은 1에서 시작) */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupQuantity(const FText& InTitle, const FText& InItemName, int32 InUnitPrice, int32 InMaxQuantity);

	virtual void KillDialog() override;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|UI")
	FOnLastFPSQuantityResult OnQuantityResult;

protected:
	// 버튼 바인딩은 인스턴스당 1회만 — CommonUI 모달은 풀에서 재사용되어
	// NativeConstruct 가 열 때마다 호출되므로 거기서 바인딩하면 누적된다.
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Cancel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Plus;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Minus;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Quantity;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_TotalPrice;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_MaxInfo;

	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleCancelClicked();
	UFUNCTION() void HandlePlusClicked();
	UFUNCTION() void HandleMinusClicked();

private:
	/** 현재 수량/합계 표시 + 증감 버튼 활성 상태 갱신 */
	void RefreshDisplay();
	void CloseWithResult(int32 Quantity);

	int32 UnitPrice = 0;
	int32 MaxQuantity = 1;
	int32 CurrentQuantity = 1;
};
