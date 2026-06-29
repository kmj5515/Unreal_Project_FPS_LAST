#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSShopData.h"
#include "LastFPSShopEntryWidget.generated.h"

class UTextBlock;
class UImage;
class ULastFPSButtonBase;

/** 구매 버튼 클릭 시 부모(상점 화면)에 행 이름을 전달 */
DECLARE_DELEGATE_OneParam(FOnShopItemBuyClicked, FName /*RowName*/);

/**
 * WBP_ShopEntry 의 Parent — 상점 목록의 한 줄.
 * Designer 바인딩(모두 선택):
 *   Image_Icon       — 아이템 아이콘
 *   Img_RarityBorder — 희귀도 색 테두리/배경
 *   TB_ItemName      — 이름
 *   TB_Description   — 설명
 *   TB_Rarity        — 희귀도 텍스트
 *   TB_Price         — 가격
 *   Button_Buy       — 구매 버튼
 */
UCLASS()
class LASTFPS_API ULastFPSShopEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 행 데이터로 표시 내용 채우기 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Shop")
	void SetupShopItem(const FLastFPSShopItemData& InItem, FName InRowName);

	/** 잔액으로 구매 가능한지에 따라 구매 버튼 활성/비활성 (무제한 재고 — 살 수 있으면 반복 구매) */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Shop")
	void SetAffordable(bool bAffordable);

	/** 이 항목의 가격 (상점 화면이 잔액 비교에 사용) */
	int32 GetPrice() const { return Price; }

	/** 구매 성공 시 호출 — 연출용(가격란 깜빡임 등). WBP에서 구현(선택). */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Shop")
	void OnPurchaseSucceeded();

	/** 부모(상점 화면)가 바인딩 — 구매 버튼 클릭 시 RowName과 함께 호출 */
	FOnShopItemBuyClicked OnBuyClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_RarityBorder;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ItemName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Description;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Rarity;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Price;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Buy;

private:
	void HandleBuyClicked();

	FName RowName;
	int32 Price = 0;
};
