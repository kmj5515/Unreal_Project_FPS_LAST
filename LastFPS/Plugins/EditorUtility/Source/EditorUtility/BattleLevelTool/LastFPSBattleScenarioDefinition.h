#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "LastFPSBattleScenarioDefinition.generated.h"

// 몬스터 그룹의 스폰 배치 방식. 스폰 지점의 방향(로컬 축)을 기준으로 배열한다.
UENUM(BlueprintType)
enum class ELastFPSBattleFormation : uint8
{
	// 좌우(스폰 지점 우측 벡터) 일렬
	Horizontal UMETA(DisplayName="가로"),
	// 앞뒤(스폰 지점 전방 벡터) 일렬
	Vertical   UMETA(DisplayName="세로"),
	// 앞뒤(행) x 좌우(열) 격자
	Grid       UMETA(DisplayName="격자")
};

USTRUCT(BlueprintType)
struct FLastFPSBattleScenarioMonsterEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster")
	TSoftObjectPtr<ULastFPSCharacterDefinition> MonsterDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster", meta=(ClampMin="1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster")
	FName SpawnTag = TEXT("EnemySpawn");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster", meta=(ClampMin="0.01"))
	float LevelScale = 1.f;

	// 스폰 배치 방식(가로/세로/격자).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster|Formation")
	ELastFPSBattleFormation Formation = ELastFPSBattleFormation::Horizontal;

	// 배치 간격(cm). 인접 몬스터 사이 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster|Formation", meta=(ClampMin="0.0"))
	float FormationSpacing = 120.f;

	// 격자 배치일 때 한 행의 열 개수. 가로/세로 배치에서는 무시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monster|Formation", meta=(ClampMin="1"))
	int32 GridColumns = 3;
};

UCLASS(BlueprintType)
class EDITORUTILITY_API ULastFPSBattleScenarioDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario")
	FName ScenarioId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario")
	TSoftObjectPtr<UWorld> BattleMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario")
	TSoftObjectPtr<ULastFPSCharacterDefinition> PlayerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scenario")
	TArray<FLastFPSBattleScenarioMonsterEntry> Monsters;
};
