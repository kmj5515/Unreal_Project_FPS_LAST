#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Character/LastFPSCharacterTypes.h"
#include "Data/Characters/LastFPSCharacterAcceleratorData.h"
#include "LastFPSCharacterMasterData.generated.h"

class APawn;
class ULastFPSAbilitySet;
class ULastFPSAIProfile;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterStatData;
class ULastFPSCharacterVisualData;
class UTexture2D;

/**
 * Editor/import source row for generating ULastFPSCharacterDefinition assets.
 * Runtime character spawning should use ULastFPSCharacterDefinition, not this table.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSCharacterMasterData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	ELastFPSCharacterType CharacterType = ELastFPSCharacterType::Player;

	/** 캐릭터의 역할과 분류를 나타내는 데이터 기반 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(Categories="Character.Type"))
	FGameplayTagContainer ClassificationTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(MultiLine=true))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Generated")
	TSoftObjectPtr<ULastFPSCharacterDefinition> TargetDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Generated")
	FString DefinitionAssetName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Generated")
	bool bGenerateDefinition = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Stats")
	TSoftObjectPtr<ULastFPSCharacterStatData> StatData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Visual")
	TSoftObjectPtr<ULastFPSCharacterVisualData> VisualData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Accelerator")
	TSoftObjectPtr<ULastFPSCharacterAcceleratorData> AcceleratorData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|GAS")
	TSoftObjectPtr<ULastFPSAbilitySet> AbilitySet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|AI")
	TSoftObjectPtr<ULastFPSAIProfile> AIProfile;
};
