#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LastFPSModuleData.generated.h"

/**
 * 모듈이 강화하는 캐릭터 스탯 종류.
 * ULastFPSAttributeSet 의 어트리뷰트와 1:1 대응 (LastFPSLoadoutSubsystem 에서 매핑).
 */
UENUM(BlueprintType)
enum class ELastFPSModuleStat : uint8
{
	MaxHealth		UMETA(DisplayName="최대 체력"),
	MaxStamina		UMETA(DisplayName="최대 스태미나"),
	AttackDamage	UMETA(DisplayName="공격력"),
	Defense			UMETA(DisplayName="방어력"),
	PhysicalDamageMultiplier UMETA(DisplayName="물리 공격 배율"),
	FireDamageMultiplier UMETA(DisplayName="화염 공격 배율"),
	IceDamageMultiplier UMETA(DisplayName="빙결 공격 배율"),
	ElectricDamageMultiplier UMETA(DisplayName="전기 공격 배율"),
	PoisonDamageMultiplier UMETA(DisplayName="독 공격 배율"),
	MoveSpeed		UMETA(DisplayName="이동속도"),
};

/** 모듈 1개가 부여하는 스탯 보정 1건 (현재 가산 only — 퍼센트는 추후 확장). */
USTRUCT(BlueprintType)
struct FLastFPSModuleStatMod
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	ELastFPSModuleStat Stat = ELastFPSModuleStat::MaxHealth;

	/** 해당 스탯에 더해질 값 (가산) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	float Value = 0.f;
};

/**
 * 모듈 1종의 기능 정의 — DataTable(DT_ModuleData) 행.
 *
 * 행 이름(RowName)은 DT_ItemData 의 동일 행 이름과 맞춘다 — 보유 판정은 EconomySubsystem.OwnedItems
 * (DT_ItemData 키)를 그대로 쓰고, 아이콘/희귀도 같은 표시는 DT_ItemData 행을 교차 참조한다.
 * 이 테이블은 "장착 시 스탯이 어떻게 바뀌는가"라는 기능 데이터만 담는다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSModuleData : public FTableRowBase
{
	GENERATED_BODY()

	/** 표시 이름 (비우면 UI가 DT_ItemData 의 ItemName 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	FText ModuleName;

	/** 이 모듈이 부여하는 스탯 보정 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module")
	TArray<FLastFPSModuleStatMod> StatMods;

	/** 장착 시 소모하는 캐파 코스트 (로드아웃 총 캐파 한도와 비교) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Module", meta=(ClampMin=0))
	int32 CapacityCost = 1;
};

/**
 * 장착 모듈 전체의 스탯 보정 합계 — UI 미리보기/적용 양쪽에서 사용.
 * (베이스 스탯은 ULastFPSCharacterStatData, 여기에 이 보정을 더한 값이 최종 스탯)
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSModuleStatTotals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Module") float MaxHealth = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float MaxStamina = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float AttackDamage = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float Defense = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float PhysicalDamageMultiplier = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float FireDamageMultiplier = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float IceDamageMultiplier = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float ElectricDamageMultiplier = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float PoisonDamageMultiplier = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Module") float MoveSpeed = 0.f;

	/** 보정이 하나라도 있는지 (적용 GE 생성 여부 판단용) */
	bool HasAny() const
	{
		return !FMath::IsNearlyZero(MaxHealth) || !FMath::IsNearlyZero(MaxStamina)
			|| !FMath::IsNearlyZero(AttackDamage) || !FMath::IsNearlyZero(Defense)
			|| !FMath::IsNearlyZero(PhysicalDamageMultiplier) || !FMath::IsNearlyZero(FireDamageMultiplier)
			|| !FMath::IsNearlyZero(IceDamageMultiplier) || !FMath::IsNearlyZero(ElectricDamageMultiplier)
			|| !FMath::IsNearlyZero(PoisonDamageMultiplier)
			|| !FMath::IsNearlyZero(MoveSpeed);
	}
};
