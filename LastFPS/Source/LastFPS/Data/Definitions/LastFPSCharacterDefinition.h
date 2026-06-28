#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/LastFPSCharacterTypes.h"
#include "LastFPSCharacterDefinition.generated.h"

class APawn;
class ULastFPSAbilitySet;
class ULastFPSAIProfile;
class ULastFPSCharacterStatData;
class ULastFPSCharacterVisualData;
class UTexture2D;

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	ELastFPSCharacterType CharacterType = ELastFPSCharacterType::Player;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FText Role;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Stats")
	TObjectPtr<ULastFPSCharacterStatData> StatData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Visual")
	TObjectPtr<ULastFPSCharacterVisualData> VisualData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|GAS")
	TObjectPtr<ULastFPSAbilitySet> AbilitySet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|AI")
	TObjectPtr<ULastFPSAIProfile> AIProfile;
};
