#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Engine/DataTable.h"
#include "LastFPSRarityVisualData.generated.h"

class UNiagaraSystem;

/**
 * 등급별 드랍 픽업 연출. 행 하나가 등급 하나를 담당한다.
 *
 * 이 매핑은 이전에 ini(DeveloperSettings) 에 경로 문자열로만 존재해 쿡 참조 그래프에 잡히지 않았고,
 * 그 결과 패키지 빌드에서만 이펙트가 통째로 빠졌다. 테이블 에셋이 소프트 참조를 소유해야
 * 쿠커가 의존성으로 따라가므로, 등급 연출은 반드시 이 테이블에서 저작한다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSRarityVisualData : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 행이 담당하는 등급. 행 이름이 아니라 이 값으로 조회하므로 행 이름은 자유롭게 지어도 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rarity")
	ELastFPSItemRarity Rarity = ELastFPSItemRarity::Common;

	/** 픽업이 스폰될 때 재생할 이펙트. 비우면 그 등급은 연출 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rarity")
	TSoftObjectPtr<UNiagaraSystem> SpawnFX;
};
