#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "LastFPSBattleScenarioDefinition.generated.h"

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
