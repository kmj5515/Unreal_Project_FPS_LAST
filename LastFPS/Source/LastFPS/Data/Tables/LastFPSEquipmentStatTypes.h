#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "LastFPSEquipmentStatTypes.generated.h"

/**
 * 모든 장비 카테고리(모듈·리액터·외장 부품)가 공유하는 스탯 종류.
 *
 * 기존 ELastFPSModuleStat 은 DT_ModuleData 행에 이미 직렬화되어 있어 이름을 바꾸면 데이터가 깨진다.
 * 그래서 모듈 열거형은 그대로 두고, 장비 전체가 합산에 사용할 상위 계약을 여기에 따로 정의한다.
 * 두 열거형 사이의 변환은 ToEquipmentStat() 하나만 거치므로 대응 관계가 한 곳에 모인다.
 *
 * 새 스탯을 추가할 때는 이 열거형과 LastFPSEquipmentStats::GetAttribute() 의 매핑 표만 손대면 되고,
 * 합산·적용·미리보기 코드는 수정할 필요가 없다.
 */
UENUM(BlueprintType)
enum class ELastFPSEquipmentStat : uint8
{
	MaxHealth                UMETA(DisplayName="최대 체력"),
	MaxStamina               UMETA(DisplayName="최대 스태미나"),
	AttackDamage             UMETA(DisplayName="공격력"),
	Defense                  UMETA(DisplayName="방어력"),
	CriticalChance           UMETA(DisplayName="치명타 확률"),
	CriticalDamagePercent    UMETA(DisplayName="치명타 피해"),
	PhysicalDamageMultiplier UMETA(DisplayName="물리 공격 배율"),
	FireDamageMultiplier     UMETA(DisplayName="화염 공격 배율"),
	IceDamageMultiplier      UMETA(DisplayName="빙결 공격 배율"),
	ElectricDamageMultiplier UMETA(DisplayName="전기 공격 배율"),
	PoisonDamageMultiplier   UMETA(DisplayName="독 공격 배율"),
	MoveSpeed                UMETA(DisplayName="이동속도"),

	Count                    UMETA(Hidden),
};

/**
 * 장비 슬롯 카테고리.
 *
 * 슬롯 위젯·선택 패널은 이 값 하나만 들고 다니며 동작하고, 카테고리별 규칙(슬롯 수, 허용 아이템 타입,
 * 데이터 테이블)은 ULastFPSEquipmentSubsystem 내부의 단일 서술 표에서 해석한다.
 *
 * 서브시스템이 아니라 이 헤더에 두는 이유: 장비 구성을 서버로 복제하는 PlayerState 가 이 열거형만
 * 필요로 하는데, GameInstanceSubsystem 헤더 전체를 끌어오지 않게 하기 위함이다.
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
 * 장착 슬롯 한 칸의 서버 제출 형태.
 *
 * 장비 상태는 소유 클라이언트의 GameInstance 에만 존재하므로, 서버가 폰에 반영하려면
 * 이 목록을 복제 경로로 받아야 한다. 스탯 값이 아니라 아이템 행 ID 만 담는다.
 * 서버가 같은 데이터 테이블에서 스탯을 다시 계산해야 클라이언트가 수치를 조작할 수 없다.
 */
USTRUCT()
struct LASTFPS_API FLastFPSEquippedSlot
{
	GENERATED_BODY()

	UPROPERTY()
	ELastFPSEquipmentSlotType SlotType = ELastFPSEquipmentSlotType::Weapon;

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY()
	FName ItemRowId;
};

/** 장비 1개가 부여하는 스탯 보정 1건 (가산). 리액터·외장 부품 테이블이 공유한다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSEquipmentStatMod
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	ELastFPSEquipmentStat Stat = ELastFPSEquipmentStat::MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	float Value = 0.f;
};

/**
 * 장착 장비 전체의 스탯 보정 합계.
 *
 * 스탯별 멤버 변수 대신 열거형 인덱스 배열로 담는다. 스탯이 늘어도 합산·비교·표시 코드가
 * 그대로 동작해야 하고, 카테고리별 기여분을 같은 방식으로 따로 담아 UI 비교에 쓰기 때문이다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSEquipmentStatTotals
{
	GENERATED_BODY()

	FLastFPSEquipmentStatTotals()
	{
		Values.Init(0.f, static_cast<int32>(ELastFPSEquipmentStat::Count));
	}

	/** 범위를 벗어난 조회는 0을 돌려주어 호출부가 매번 검사하지 않아도 되게 한다. */
	float GetStat(ELastFPSEquipmentStat Stat) const
	{
		const int32 Index = static_cast<int32>(Stat);
		return Values.IsValidIndex(Index) ? Values[Index] : 0.f;
	}

	void AddStat(ELastFPSEquipmentStat Stat, float Value)
	{
		const int32 Index = static_cast<int32>(Stat);
		if (Values.IsValidIndex(Index))
		{
			Values[Index] += Value;
		}
	}

	void Append(const FLastFPSEquipmentStatTotals& Other)
	{
		const int32 Num = FMath::Min(Values.Num(), Other.Values.Num());
		for (int32 Index = 0; Index < Num; ++Index)
		{
			Values[Index] += Other.Values[Index];
		}
	}

	/** 보정이 하나라도 있는지 — 적용 GE 를 만들지 판단한다. */
	bool HasAny() const
	{
		for (const float Value : Values)
		{
			if (!FMath::IsNearlyZero(Value))
			{
				return true;
			}
		}
		return false;
	}

private:
	/** 인덱스 = ELastFPSEquipmentStat. 생성자에서 Count 크기로 채운다. */
	UPROPERTY()
	TArray<float> Values;
};

namespace LastFPSEquipmentStats
{
	/** 모듈 테이블의 기존 열거형을 장비 공용 열거형으로 옮긴다. 대응이 없으면 false. */
	LASTFPS_API bool ToEquipmentStat(ELastFPSModuleStat ModuleStat, ELastFPSEquipmentStat& OutStat);

	/** UI 표시용 이름. 열거형 메타의 DisplayName 을 그대로 읽어 표를 중복 저작하지 않는다. */
	LASTFPS_API FText GetDisplayName(ELastFPSEquipmentStat Stat);

	/**
	 * 값 표시 형식이 스탯마다 다르다(확률·배율은 백분율, 체력·공격력은 정수).
	 * 표시 규칙을 한 곳에 모아 요약 패널과 비교 프리뷰가 같은 문자열을 쓰게 한다.
	 */
	LASTFPS_API FText FormatValue(ELastFPSEquipmentStat Stat, float Value, bool bShowSign = false);
}
