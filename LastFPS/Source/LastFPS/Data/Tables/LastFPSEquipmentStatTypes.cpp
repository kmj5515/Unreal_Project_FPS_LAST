#include "Data/Tables/LastFPSEquipmentStatTypes.h"

#include "Internationalization/Text.h"
#include "Localization/LastFPSLocalization.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace LastFPSEquipmentStats
{
namespace
{
	/**
	 * 배율 계열은 1.0 을 기준으로 가산되는 값이라 0.15 를 "+15%" 로 읽어야 의미가 통한다.
	 * 반면 CriticalChance(0~100) 와 CriticalDamagePercent(100~) 는 이미 백분율 단위로
	 * AttributeSet 에 저장되므로 배율을 곱하면 안 된다. 두 규칙을 구분하는 판정을 한 곳에 둔다.
	 */
	bool IsRatioStat(ELastFPSEquipmentStat Stat)
	{
		switch (Stat)
		{
		case ELastFPSEquipmentStat::PhysicalDamageMultiplier:
		case ELastFPSEquipmentStat::FireDamageMultiplier:
		case ELastFPSEquipmentStat::IceDamageMultiplier:
		case ELastFPSEquipmentStat::ElectricDamageMultiplier:
		case ELastFPSEquipmentStat::PoisonDamageMultiplier:
			return true;
		default:
			return false;
		}
	}

	bool IsPercentPointStat(ELastFPSEquipmentStat Stat)
	{
		return Stat == ELastFPSEquipmentStat::CriticalChance
			|| Stat == ELastFPSEquipmentStat::CriticalDamagePercent;
	}
}

bool ToEquipmentStat(ELastFPSModuleStat ModuleStat, ELastFPSEquipmentStat& OutStat)
{
	switch (ModuleStat)
	{
	case ELastFPSModuleStat::MaxHealth:                OutStat = ELastFPSEquipmentStat::MaxHealth;                return true;
	case ELastFPSModuleStat::MaxStamina:               OutStat = ELastFPSEquipmentStat::MaxStamina;               return true;
	case ELastFPSModuleStat::AttackDamage:             OutStat = ELastFPSEquipmentStat::AttackDamage;             return true;
	case ELastFPSModuleStat::Defense:                  OutStat = ELastFPSEquipmentStat::Defense;                  return true;
	case ELastFPSModuleStat::PhysicalDamageMultiplier: OutStat = ELastFPSEquipmentStat::PhysicalDamageMultiplier; return true;
	case ELastFPSModuleStat::FireDamageMultiplier:     OutStat = ELastFPSEquipmentStat::FireDamageMultiplier;     return true;
	case ELastFPSModuleStat::IceDamageMultiplier:      OutStat = ELastFPSEquipmentStat::IceDamageMultiplier;      return true;
	case ELastFPSModuleStat::ElectricDamageMultiplier: OutStat = ELastFPSEquipmentStat::ElectricDamageMultiplier; return true;
	case ELastFPSModuleStat::PoisonDamageMultiplier:   OutStat = ELastFPSEquipmentStat::PoisonDamageMultiplier;   return true;
	case ELastFPSModuleStat::MoveSpeed:                OutStat = ELastFPSEquipmentStat::MoveSpeed;                return true;
	default:
		return false;
	}
}

FText GetDisplayName(ELastFPSEquipmentStat Stat)
{
	return FLastFPSLocalization::GetUIEnumText(
		StaticEnum<ELastFPSEquipmentStat>(),
		static_cast<int64>(Stat));
}

FText FormatValue(ELastFPSEquipmentStat Stat, float Value, bool bShowSign)
{
	FNumberFormattingOptions Options;
	Options.SetUseGrouping(true);

	float DisplayValue = Value;
	bool bIsPercent = false;

	if (IsRatioStat(Stat))
	{
		DisplayValue = Value * 100.f;
		Options.SetMaximumFractionalDigits(1);
		bIsPercent = true;
	}
	else if (IsPercentPointStat(Stat))
	{
		Options.SetMaximumFractionalDigits(1);
		bIsPercent = true;
	}
	else
	{
		Options.SetMaximumFractionalDigits(0);
	}

	const FText Number = FText::AsNumber(DisplayValue, &Options);
	// 비교 프리뷰는 증감 방향이 보여야 하므로 양수에도 부호를 붙일 수 있게 한다.
	const FText SignedNumber = (bShowSign && DisplayValue > 0.f)
		? FText::Format(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::PositiveValueFormat), Number)
		: Number;

	return bIsPercent
		? FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::PercentFormat),
			SignedNumber)
		: SignedNumber;
}
}
