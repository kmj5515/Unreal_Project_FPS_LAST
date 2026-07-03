#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "LastFPSLoadoutSubsystem.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class ULastFPSEconomySubsystem;

/** 장착 모듈 구성이 바뀔 때 (장착/해제) 브로드캐스트 — 로드아웃/인벤토리 UI 갱신용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSLoadoutChanged);

/**
 * 아웃게임 모듈 로드아웃 — GameInstanceSubsystem.
 *
 * 장착 모듈(슬롯별 DT_ModuleData 행 이름)을 보관하는 **메타 상태**다. 맵 이동에도 유지되며
 * (SaveGame 영속화는 Step 2 예정), 전투 폰 스폰 시 ApplyToAbilitySystem() 으로 AttributeSet 에
 * 합산 보정을 Infinite GE 로 적용한다. ComputeBonus() 는 UI 스탯 미리보기에 쓴다.
 *
 * 장착 가능 조건: ① 슬롯 인덱스 유효 ② 해당 모듈을 보유(EconomySubsystem) ③ 총 캐파 한도 이내.
 * 범위 메모: 캐릭터 1개 공유 로드아웃(계승자별 분리는 추후). 강화/세트효과 없음.
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSLoadoutSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	int32 GetSlotCount() const { return SlotCount; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	int32 GetMaxCapacity() const { return MaxCapacity; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	int32 GetUsedCapacity() const;

	/** 슬롯에 장착된 모듈 행 이름 (없으면 NAME_None) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	FName GetEquippedModule(int32 SlotIndex) const;

	const TArray<FName>& GetEquippedModules() const { return EquippedModules; }

	/**
	 * 슬롯에 모듈 장착 시도. 보유/캐파/슬롯 검증 통과 시 장착하고 true.
	 * 기존 장착분은 교체된다(교체 후 캐파로 한도 검사). 실패 시 아무것도 바꾸지 않음.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Loadout")
	bool TryEquip(int32 SlotIndex, FName ModuleRowId);

	/** 슬롯 비우기 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Loadout")
	void Unequip(int32 SlotIndex);

	/** 특정 모듈을 (교체 포함) 장착하면 캐파 한도를 넘는지 — UI 활성/비활성 판단용 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	bool CanEquip(int32 SlotIndex, FName ModuleRowId) const;

	/** DT_ModuleData 에서 행 조회 (없으면 nullptr) */
	const FLastFPSModuleData* FindModule(FName ModuleRowId) const;

	/** 장착 모듈 전체의 스탯 보정 합 (UI 미리보기용) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Loadout")
	FLastFPSModuleStatTotals ComputeBonus() const;

	/**
	 * 합산 보정을 ASC 에 Infinite GameplayEffect 로 적용 (서버 권위에서 폰 스폰 시 호출).
	 * 베이스 스탯(ULastFPSCharacterStatData)이 SetNumericAttributeBase 로 적용된 "뒤"에 호출할 것.
	 */
	void ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Loadout")
	FOnLastFPSLoadoutChanged OnLoadoutChanged;

protected:
	/** 모듈 슬롯 개수 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Loadout", meta=(ClampMin=1))
	int32 SlotCount = 4;

	/** 장착 모듈 캐파 코스트 합의 한도 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Loadout", meta=(ClampMin=0))
	int32 MaxCapacity = 10;

	/** 모듈 기능 정의 테이블 (DT_ModuleData) — DefaultGame.ini 로 지정 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Loadout")
	TSoftObjectPtr<UDataTable> ModuleTable;

private:
	ULastFPSEconomySubsystem* GetEconomy() const;

	/**
	 * 시작 시 DT_ModuleData 의 모든 행이 DT_ItemData 에 대응 행을 갖는지 검증(에러 로그).
	 * 대응 행이 없는 모듈은 소유 판정/표시가 불가능해 인벤토리에 나타나지 않으므로 조기 경고.
	 */
	void ValidateModuleReferences() const;

	/** 슬롯별 장착 모듈 행 이름 (크기 = SlotCount, 빈 칸 = NAME_None) */
	UPROPERTY()
	TArray<FName> EquippedModules;
};
