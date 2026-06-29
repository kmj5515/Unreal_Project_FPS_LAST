#pragma once

#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "LastFPSShopScreenWidget.generated.h"

class UDataTable;
class UPanelWidget;
class UTextBlock;
class ULastFPSShopEntryWidget;
class ULastFPSEconomySubsystem;
struct FLastFPSShopItemData;

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

	/** 아이템 정의 테이블 (RowType = FLastFPSItemData) — 스택 한도(MaxStackSize) 조회용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(RequiredAssetDataTags="RowStructure=/Script/LastFPS.LastFPSItemData"))
	TObjectPtr<UDataTable> ItemTable;

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

	/**
	 * 이 항목을 지금 몇 개까지 살 수 있는지 = min(잔액/단가, 스택여유).
	 * 스택여유 = MaxStackSize - 현재보유. 무기 등 MaxStackSize=1 은 이미 보유 시 0(구매 불가).
	 * (지급 아이템이 없는 화폐성 구매는 잔액 한도만 적용.)
	 */
	int32 ComputeMaxPurchasable(const FLastFPSShopItemData& Row) const;

	/** 구매 버튼 클릭 — 구매 가능 수량을 계산해 수량 선택 모달을 띄움 */
	void HandleBuyRequested(FName RowName, ULastFPSShopEntryWidget* Entry);

	/** 수량 모달 결과 — Quantity>0 이면 그 수량만큼 구매 확정 */
	UFUNCTION()
	void HandleQuantityChosen(int32 Quantity);

	/** 잔액 변동 시 — 잔액 텍스트 갱신 + 각 엔트리 구매 가능 여부 재평가 */
	UFUNCTION()
	void HandleCreditsChanged(int32 NewCredits);

	/** 보유 변동 시 — 스택이 찬 항목의 구매 버튼을 비활성화하도록 재평가 */
	UFUNCTION()
	void HandleInventoryChanged();

	/** 모든 엔트리의 구매 가능 여부(잔액+스택)를 현재 상태로 갱신 */
	void RefreshEntryStates();

	/** RowName → 생성된 엔트리 (잔액/보유 변동 시 일괄 갱신용) */
	UPROPERTY()
	TMap<FName, TObjectPtr<ULastFPSShopEntryWidget>> EntryByRow;

	/** 수량 모달이 열려 있는 동안 어떤 항목을 구매 중인지 보관 (모달은 한 번에 하나) */
	FName PendingRowName;
	TWeakObjectPtr<ULastFPSShopEntryWidget> PendingEntry;
};
