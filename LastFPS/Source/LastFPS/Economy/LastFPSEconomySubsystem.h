#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSEconomySubsystem.generated.h"

class UDataTable;
enum class ELastFPSItemRarity : uint8;
struct FLastFPSRarityVisualData;

/** 성공한 상점 구매의 불변 영수증. 구매 외 아이템 증가와 퀘스트 판정을 분리하는 데이터 계약이다. */
struct LASTFPS_API FLastFPSPurchaseReceipt
{
	FName ItemRowId;
	int32 Quantity = 0;
	int32 UnitPrice = 0;
	int32 TotalCost = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLastFPSPurchaseCommitted, const FLastFPSPurchaseReceipt&);

/** 잔액 변동 시 브로드캐스트 (상점 잔액 표시 / 구매 버튼 활성화 갱신용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSCreditsChanged, int32, NewCredits);

/** 보유 아이템 변동 시 브로드캐스트 (인벤토리 화면 재구성용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSInventoryChanged);

/**
 * 아웃게임 경제(화폐/보유 아이템) — GameInstanceSubsystem.
 * 맵 이동(메인메뉴/캐릭선택/허브)에도 살아남아 세션 동안 잔액과 보유 아이템을 유지한다.
 * (앱 재시작 시 StartingCredits/StartingOwnedItems 로 초기화 — SaveGame 영속화는 추후.)
 *
 * 상점: TryPurchase() 로 잔액 차감 + 아이템 지급. 인벤토리: GetOwnedItems() 를 읽어 표시.
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="LastFPS|Economy")
	int32 GetCredits() const { return Credits; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Economy")
	bool CanAfford(int32 Price) const { return Credits >= FMath::Max(0, Price); }

	/** 잔액 증가(보상/환불/디버그). 0 이하 무시. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Economy")
	void AddCredits(int32 Amount);

	/**
	 * 구매 시도 — 잔액이 단가(Price)×수량(Count) 이상이면 그만큼 차감하고 GrantItemRowId 아이템을 Count개 지급.
	 * 잔액 부족 시 아무것도 바꾸지 않고 false. (GrantItemRowId 비면 화폐만 차감.) Count<=0 은 1로 취급.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Economy")
	bool TryPurchase(FName GrantItemRowId, int32 Price, int32 Count = 1);

	/** 보유 아이템 직접 추가(보상/디버그). RowId 비거나 Count<=0 이면 무시. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Economy")
	void AddItem(FName ItemRowId, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category="LastFPS|Economy")
	int32 GetItemCount(FName ItemRowId) const;

	/** 보유 아이템 전체 (DT_ItemData 행 이름 → 수량) — 인벤토리 화면이 읽음 */
	const TMap<FName, int32>& GetOwnedItems() const { return OwnedItems; }

	/** RowId가 DT_ItemData에 실제 정의된 아이템인지. GameDataSet 로드 실패 시 false. */
	bool HasItemDefinition(FName ItemRowId) const;

	/** RowId 의 표시 이름 (DT_ItemData.ItemName). 정의가 없으면 RowId 문자열로 폴백. */
	FText GetItemDisplayName(FName ItemRowId) const;

	/** RowId 의 등급을 조회. 정의가 있으면 OutRarity 채우고 true, 없으면 false(OutRarity 미변경). */
	bool TryGetItemRarity(FName ItemRowId, ELastFPSItemRarity& OutRarity) const;

	/** DT_ItemData가 GameDataSet을 통해 로드됐는지 반환한다. */
	bool IsItemTableConfigured() const;

	/**
	 * 등급의 드랍 연출 행을 조회한다. 테이블이 없거나 해당 등급 행이 없으면 nullptr.
	 * 등급 데이터의 해석은 이 서브시스템이 소유하므로 픽업 액터는 테이블을 직접 알지 않는다.
	 */
	const FLastFPSRarityVisualData* FindRarityVisual(ELastFPSItemRarity Rarity) const;

	/**
	 * 시작 시 테이블 간 참조 무결성 검사 — 깨진 참조를 에러 로그로 출력한다.
	 * DT_ShopData 의 GrantItemRowId, StartingOwnedItems 키가 DT_ItemData 에 존재하는지 확인.
	 * 모듈 테이블 검증은 LoadoutSubsystem이 담당한다.
	 */
	void ValidateReferences() const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Economy")
	FOnLastFPSCreditsChanged OnCreditsChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Economy")
	FOnLastFPSInventoryChanged OnInventoryChanged;

	/** 결제 성공 후 아이템 보유량 변경 전에 발행한다. C++ 퀘스트 시스템이 실제 구매액만 추적하는 계약이다. */
	FOnLastFPSPurchaseCommitted OnPurchaseCommitted;

protected:
	/** 세션 시작 잔액 (DefaultGame.ini 로 조정 가능) */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Economy")
	int32 StartingCredits = 10000;

	/** 세션 시작 보유 아이템 (DT_ItemData 행 이름 → 수량). 데모 시드용. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Economy")
	TMap<FName, int32> StartingOwnedItems;

private:
	/** GameDataSet에서 DT_ItemData를 조회한다. */
	const UDataTable* GetItemTable() const;

	int32 Credits = 0;

	/** DT_ItemData 행 이름 → 보유 수량 */
	TMap<FName, int32> OwnedItems;
};
