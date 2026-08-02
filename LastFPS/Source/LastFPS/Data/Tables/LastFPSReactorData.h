#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSReactorData.generated.h"

/**
 * 리액터 1종의 기능 정의 — DataTable(DT_ReactorData) 행.
 *
 * 행 이름(RowName)은 DT_ItemData 의 동일 행 이름과 맞춘다. 모듈 테이블과 같은 규칙으로,
 * 보유 판정은 EconomySubsystem(DT_ItemData 키), 아이콘·희귀도 같은 표시는 DT_ItemData 를 교차 참조한다.
 * 이 테이블은 "장착 시 무엇이 강해지는가"라는 기능 데이터만 담는다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSReactorData : public FTableRowBase
{
	GENERATED_BODY()

	/** 표시 이름 (비우면 UI 가 DT_ItemData 의 ItemName 을 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactor")
	FText ReactorName;

	/**
	 * 이 리액터가 강화하는 속성. 스킬·무기 속성과 맞물릴 때 UI 가 강조 표시하는 데 쓰며,
	 * 실제 배율 증가는 StatMods 로 저작한다(속성 하나에 배율이 고정되지 않도록 분리).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactor")
	ELastFPSDamageElement ElementType = ELastFPSDamageElement::Physical;

	/** 장착 시 부여하는 스탯 보정 목록 (모듈·외장 부품과 동일한 공용 계약) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactor")
	TArray<FLastFPSEquipmentStatMod> StatMods;

	/**
	 * 스킬 위력 배율 가산치. AttributeSet 에 대응 어트리뷰트가 아직 없어 표시 전용이며,
	 * 스킬 계수 시스템이 들어오면 StatMods 로 옮기고 이 필드는 제거한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactor")
	float SkillPowerBonus = 0.f;

	/** 스킬 쿨다운 감소 비율(0~1). 위와 같은 이유로 현재는 표시 전용이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactor", meta=(ClampMin=0, ClampMax=1))
	float SkillCooldownReduction = 0.f;
};
