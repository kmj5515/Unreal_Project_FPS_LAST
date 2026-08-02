#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSEquipmentSubsystem.generated.h"

class UAbilitySystemComponent;
class UDataTable;
class ULastFPSEconomySubsystem;
class ULastFPSLoadoutSubsystem;
class ULastFPSWeaponDefinition;

/**
 * 장비 슬롯 카테고리.
 *
 * 슬롯 위젯·선택 패널은 이 값 하나만 들고 다니며 동작하고, 카테고리별 규칙(슬롯 수, 허용 아이템 타입,
 * 데이터 테이블)은 서브시스템 내부의 단일 서술 표에서 해석한다. 카테고리를 늘릴 때 UI 코드는 손대지 않는다.
 */
UENUM(BlueprintType)
enum class ELastFPSEquipmentSlotType : uint8
{
	Weapon            UMETA(DisplayName="무기"),
	Reactor           UMETA(DisplayName="리액터"),
	ExternalComponent UMETA(DisplayName="외장 부품"),
	Module            UMETA(DisplayName="모듈"),

	Count             UMETA(Hidden),
};

/**
 * 장비 구성이 바뀔 때 브로드캐스트한다.
 *
 * 카테고리마다 델리게이트를 따로 두지 않고 슬롯 타입을 인자로 넘긴다. 구독자(장비 화면·HUD·스탯 요약)는
 * 대부분 전체 재구성을 하거나 관심 있는 타입만 걸러 쓰면 되고, 카테고리를 추가할 때 델리게이트와
 * 바인딩 코드가 함께 늘어나는 것을 막기 위해서다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLastFPSEquipmentChanged, ELastFPSEquipmentSlotType, SlotType, int32, SlotIndex);

/**
 * 통합 장비 로드아웃 — GameInstanceSubsystem.
 *
 * 무기 2슬롯 / 리액터 1슬롯 / 외장 부품 4슬롯을 직접 소유하고, 모듈 슬롯은 기존
 * ULastFPSLoadoutSubsystem 에 위임한다(모듈 캐파 규칙과 기존 화면을 그대로 살리기 위함).
 * 맵 이동에도 유지되는 **메타 상태**이며, 전투 폰 스폰 시 서버가 무기 로드아웃을 WeaponComponent 에,
 * 스탯 합계를 ApplyToAbilitySystem() 으로 ASC 에 적용한다.
 *
 * 장착 조건: ① 슬롯 유효 ② 아이템 타입이 슬롯 카테고리와 일치 ③ 보유 수량 이내
 *            ④ 카테고리별 추가 규칙(외장 부품의 지정 슬롯 등).
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSEquipmentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	int32 GetSlotCount(ELastFPSEquipmentSlotType SlotType) const;

	/** 슬롯에 장착된 아이템 행 이름 (없으면 NAME_None) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	FName GetEquippedItem(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex) const;

	/** 장착 가능 여부 — UI 활성/비활성 판단과 TryEquip 의 단일 검증 경로를 겸한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	bool CanEquip(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId) const;

	/** 검증 통과 시 장착하고 true. 기존 장착분은 교체된다. 실패 시 아무것도 바꾸지 않는다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Equipment")
	bool TryEquip(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Equipment")
	void Unequip(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex);

	/** 이 카테고리에 장착할 수 있는 아이템 타입 — 선택 패널이 보유 목록을 거를 때 쓴다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	ELastFPSItemType GetAcceptedItemType(ELastFPSEquipmentSlotType SlotType) const;

	/** 무기 슬롯에 장착된 무기 정의. 비었거나 참조가 깨졌으면 null. */
	ULastFPSWeaponDefinition* GetWeaponDefinitionForSlot(int32 SlotIndex) const;

	/** 전투에 사용할 수 있는 무기가 하나 이상 장착되어 있는지 반환한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	bool HasEquippedWeapon() const;

	/** 모든 카테고리 합산 보정 (스탯 요약·GE 적용의 단일 출처) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	FLastFPSEquipmentStatTotals ComputeTotals() const;

	/** 카테고리 하나의 기여분 — 요약 패널이 출처별로 색을 나눠 표시할 때 쓴다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	FLastFPSEquipmentStatTotals ComputeCategoryTotals(ELastFPSEquipmentSlotType SlotType) const;

	/**
	 * 지정 슬롯에 후보 아이템을 끼웠다고 가정한 합계.
	 * 실제 상태를 바꾸지 않고 장착 전후를 비교하기 위한 조회이며, 선택 패널의 ▲▼ 프리뷰가 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Equipment")
	FLastFPSEquipmentStatTotals ComputeTotalsWithCandidate(
		ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName CandidateItemRowId) const;

	/**
	 * 합산 보정을 ASC 에 Infinite GameplayEffect 로 적용 (서버 권위에서 폰 스폰 시 호출).
	 * 베이스 스탯(ULastFPSCharacterStatData)이 적용된 "뒤"에 호출할 것.
	 * 모든 장비 카테고리를 하나의 GE 로 합쳐 적용 지점을 단일화한다.
	 */
	void ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Equipment")
	FOnLastFPSEquipmentChanged OnEquipmentChanged;

protected:
	/** 주무기 / 보조무기 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Equipment", meta=(ClampMin=1))
	int32 WeaponSlotCount = 2;

	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Equipment", meta=(ClampMin=1))
	int32 ReactorSlotCount = 1;

	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Equipment", meta=(ClampMin=1))
	int32 ExternalComponentSlotCount = 4;

private:
	/** 모듈 로드아웃의 직접 변경도 통합 장비 변경 알림으로 전달한다. */
	UFUNCTION()
	void HandleModuleLoadoutChanged();

	ULastFPSEconomySubsystem* GetEconomy() const;
	ULastFPSLoadoutSubsystem* GetModuleLoadout() const;
	const UDataTable* GetItemTable() const;
	const FLastFPSItemData* FindItem(FName ItemRowId) const;

	/** 모듈은 LoadoutSubsystem 이 소유하므로 자체 배열을 갖는 카테고리만 여기서 찾는다. */
	TArray<FName>* FindOwnedSlotArray(ELastFPSEquipmentSlotType SlotType);
	const TArray<FName>* FindOwnedSlotArray(ELastFPSEquipmentSlotType SlotType) const;

	/** 카테고리별 추가 규칙. 외장 부품의 지정 슬롯처럼 데이터가 정한 제약을 확인한다. */
	bool PassesCategoryRule(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId) const;

	/**
	 * 같은 아이템을 보유 수량보다 많은 슬롯에 끼우지 못하게 한다.
	 * @param IgnoredSlotIndex 교체 대상 슬롯. 자기 자신은 세지 않아야 재장착이 막히지 않는다.
	 */
	bool HasSpareCopy(ELastFPSEquipmentSlotType SlotType, int32 IgnoredSlotIndex, FName ItemRowId) const;

	/** 한 슬롯이 부여하는 보정. 카테고리별 테이블을 읽어 공용 계약으로 옮긴다. */
	void AccumulateSlotStats(
		ELastFPSEquipmentSlotType SlotType, FName ItemRowId, FLastFPSEquipmentStatTotals& OutTotals) const;

	/** 시작 시 리액터·외장 테이블 행이 DT_ItemData 에 대응 행을 갖는지 검증(에러 로그). */
	void ValidateEquipmentReferences() const;

	UPROPERTY()
	TArray<FName> WeaponSlots;

	UPROPERTY()
	TArray<FName> ReactorSlots;

	UPROPERTY()
	TArray<FName> ExternalComponentSlots;
};
