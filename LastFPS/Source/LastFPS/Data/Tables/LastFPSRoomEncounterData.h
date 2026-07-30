#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSRoomEncounterData.generated.h"

class ULastFPSCharacterDefinition;
class ULastFPSEncounterObjectiveDefinition;
class UNiagaraSystem;

/**
 * 이 방이 요구하는 목표 1건이다.
 *
 * 구조체로 감싼 이유: 목표 활성 시점(웨이브 연동·선행 목표 등)이 나중에 필요해질 때
 * 필드를 추가만 하면 되고 기존 행이 그대로 살기 때문이다. 소프트 참조 배열로 두면
 * 그때 배열 원소 타입이 바뀌어 데이터 마이그레이션이 필요하다.
 */
USTRUCT(BlueprintType)
struct FLastFPSEncounterObjectiveEntry
{
	GENERATED_BODY()

	/** 목표의 종류와 밸런스를 소유하는 정의 에셋이다. 위치는 레벨 배치물이 소유한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TSoftObjectPtr<ULastFPSEncounterObjectiveDefinition> Definition;
};

/** 룸 인카운터에서 적 생성 순간에 재생할 시각 효과 설정이다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomEncounterSpawnVFXDefinition
{
	GENERATED_BODY()

	/** 비어 있으면 생성 시각 효과를 재생하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawn VFX")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** 적 생성 Transform의 로컬 공간을 기준으로 적용할 위치 오프셋이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawn VFX", meta=(Units="cm"))
	FVector LocationOffset = FVector::ZeroVector;

	/** 적 생성 회전에 추가로 적용할 회전 오프셋이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawn VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** Niagara System에 적용할 월드 스케일이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawn VFX")
	FVector Scale = FVector::OneVector;

	/** 스폰 이펙트를 재생한 뒤 실제 적을 생성하기까지 기다리는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawn VFX", meta=(ClampMin="0.0", Units="s"))
	float DelayBeforeSpawn = 0.8f;

	bool IsValid(FString& OutFailureReason) const;
};

/** 한 웨이브에 생성할 적 종류와 수량이다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomEncounterUnitEntry
{
	GENERATED_BODY()

	/** 생성할 적의 데이터 정의다. Pawn 클래스와 능력치 등은 Character Definition이 소유한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TSoftObjectPtr<ULastFPSCharacterDefinition> EnemyDefinition;

	/** 이 웨이브에서 해당 적을 생성할 수량이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="1"))
	int32 Count = 1;
};

/** 순서대로 실행되는 단일 웨이브의 구성이다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomEncounterWaveDefinition
{
	GENERATED_BODY()

	/** 이전 웨이브 종료 또는 전투 시작 후 이 웨이브가 시작되기까지의 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="0.0", Units="s"))
	float DelayBeforeWave = 0.f;

	/** 한 적을 생성한 뒤 다음 적을 생성하기까지의 시간이다. 0이면 웨이브의 모든 적을 즉시 생성한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="0.0", Units="s"))
	float SpawnInterval = 0.35f;

	/** 한 생성 간격마다 동시에 생성할 적 수다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="1"))
	int32 SpawnBatchSize = 3;

	/** 웨이브가 시작될 때 Spawn Point 사용 순서를 섞어 여러 방향에서 생성되게 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	bool bShuffleSpawnPoints = true;

	/** 같은 웨이브에 생성할 적 종류들이다. 순차 생성 시 배열 순서대로 처리한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(TitleProperty="EnemyDefinition"))
	TArray<FLastFPSRoomEncounterUnitEntry> Units;

	bool IsValid(FString& OutFailureReason) const;
};

/** Data Table의 한 행이며, Row Name은 레벨 Actor의 Encounter 식별 태그와 일치해야 한다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomEncounterData : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * 이 방이 속한 목적지 식별자다(DT_Destination 행 이름).
	 * 프로파일이 자기 목적지와 다른 행을 프리로드·초기화에서 건너뛰는 기준이며,
	 * 비워 두면 목적지를 가리지 않는다(기존 행 호환).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	FName DestinationId;

	/** 실행 순서대로 구성한 웨이브 목록이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<FLastFPSRoomEncounterWaveDefinition> Waves;

	/**
	 * 이 방이 요구하는 목표들이다. 비우면 섬멸형(웨이브를 모두 소진하면 클리어)이다.
	 * 목표가 하나라도 있으면 전부 성공할 때까지 웨이브를 순환하며 클리어를 보류한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<FLastFPSEncounterObjectiveEntry> Objectives;

	/** Spawn Point 수보다 유닛이 많을 때 재사용 위치에 적용할 간격이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="0.0", Units="cm"))
	float ReusedSpawnPointSpacing = 180.f;

	/** 각 Spawn Point를 중심으로 무작위 NavMesh 생성 위치를 찾을 반경이다. 0이면 정확한 Spawn Point를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="0.0", Units="cm"))
	float SpawnPointRandomRadius = 600.f;

	/** 이 인카운터에서 공통으로 사용할 적 생성 시각 효과다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	FLastFPSRoomEncounterSpawnVFXDefinition SpawnVFX;

	/** 모든 웨이브에 정의된 적 수의 합을 반환한다. 퀘스트 진행 표시도 이 값을 단일 기준으로 사용한다. */
	int32 GetTotalEnemyCount() const;

	bool IsValid(FString& OutFailureReason) const;
};
