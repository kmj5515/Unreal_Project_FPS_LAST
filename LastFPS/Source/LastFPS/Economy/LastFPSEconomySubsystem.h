#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSEconomySubsystem.generated.h"

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
	 * 구매 시도 — 잔액이 충분하면 Price 만큼 차감하고 GrantItemRowId 아이템을 1개 지급.
	 * 잔액 부족 시 아무것도 바꾸지 않고 false. (GrantItemRowId 비면 화폐만 차감.)
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Economy")
	bool TryPurchase(FName GrantItemRowId, int32 Price);

	/** 보유 아이템 직접 추가(보상/디버그). RowId 비거나 Count<=0 이면 무시. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Economy")
	void AddItem(FName ItemRowId, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category="LastFPS|Economy")
	int32 GetItemCount(FName ItemRowId) const;

	/** 보유 아이템 전체 (DT_ItemData 행 이름 → 수량) — 인벤토리 화면이 읽음 */
	const TMap<FName, int32>& GetOwnedItems() const { return OwnedItems; }

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Economy")
	FOnLastFPSCreditsChanged OnCreditsChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Economy")
	FOnLastFPSInventoryChanged OnInventoryChanged;

protected:
	/** 세션 시작 잔액 (DefaultGame.ini 로 조정 가능) */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Economy")
	int32 StartingCredits = 10000;

	/** 세션 시작 보유 아이템 (DT_ItemData 행 이름 → 수량). 데모 시드용. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Economy")
	TMap<FName, int32> StartingOwnedItems;

private:
	int32 Credits = 0;

	/** DT_ItemData 행 이름 → 보유 수량 */
	TMap<FName, int32> OwnedItems;
};
