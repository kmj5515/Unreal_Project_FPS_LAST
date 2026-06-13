#pragma once

#include "UI/LastFPSContentScreenWidget.h"
#include "LastFPSShopScreenWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UTextBlock;
class ULastFPSShopEntryWidget;
class ULastFPSEconomySubsystem;

/**
 * 상점 화면 — ContentScreen 크롬(타이틀/닫기) 위에 판매 항목을 나열.
 * ShopTable의 모든 행을 EntryWidgetClass 인스턴스로 만들어 Box_ShopList에 채운다.
 * 화폐/재고 시스템은 아직 없음 — 구매 버튼은 항목별로 "구매됨" 상태만 토글(프로토).
 */
UCLASS()
class LASTFPS_API ULastFPSShopScreenWidget : public ULastFPSContentScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 판매 항목 테이블 (RowType = FLastFPSShopItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSShopItemData"))
	TObjectPtr<UDataTable> ShopTable;

	/** 행 하나를 그릴 위젯 클래스 (WBP_ShopEntry) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	TSubclassOf<ULastFPSShopEntryWidget> EntryWidgetClass;

	/** 엔트리를 담을 컨테이너 (VerticalBox / ScrollBox 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_ShopList;

	/** 행이 하나도 없을 때 보일 안내 텍스트 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/** 현재 보유 잔액 표시 (선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Credits;

	/** 테이블을 다시 읽어 목록을 재구성 */
	UFUNCTION(BlueprintCallable, Category="Shop")
	void RebuildShopList();

private:
	ULastFPSEconomySubsystem* GetEconomy() const;

	/** 엔트리의 구매 버튼 클릭 처리 — 잔액 차감 + 아이템 지급(TryPurchase) */
	void HandleItemPurchased(FName RowName, ULastFPSShopEntryWidget* Entry);

	/** 잔액 변동 시 — 잔액 텍스트 갱신 + 각 엔트리 구매 가능 여부 재평가 */
	UFUNCTION()
	void HandleCreditsChanged(int32 NewCredits);

	/** RowName → 생성된 엔트리 (잔액 변동 시 일괄 갱신용) */
	UPROPERTY()
	TMap<FName, TObjectPtr<ULastFPSShopEntryWidget>> EntryByRow;
};
