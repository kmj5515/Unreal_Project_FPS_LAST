#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "LastFPSExternalComponentData.generated.h"

/**
 * 외장 부품 1종의 기능 정의 — DataTable(DT_ExternalComponentData) 행.
 *
 * 행 이름(RowName)은 DT_ItemData 의 동일 행 이름과 맞춘다(모듈·리액터와 동일 규칙).
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSExternalComponentData : public FTableRowBase
{
	GENERATED_BODY()

	/** 표시 이름 (비우면 UI 가 DT_ItemData 의 ItemName 을 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="External Component")
	FText ComponentName;

	/**
	 * 이 부품이 들어갈 수 있는 슬롯 번호(1~4). 0 이면 어느 슬롯에나 장착할 수 있다.
	 * 부위별로 착용 위치를 고정하는 규칙을 코드가 아니라 데이터로 표현하기 위한 필드다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="External Component", meta=(ClampMin=0, ClampMax=4))
	int32 SlotIndex = 0;

	/** 장착 시 부여하는 스탯 보정 목록 (모듈·리액터와 동일한 공용 계약) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="External Component")
	TArray<FLastFPSEquipmentStatMod> StatMods;
};
