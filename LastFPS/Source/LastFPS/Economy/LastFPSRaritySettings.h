#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Data/Tables/LastFPSItemData.h"
#include "LastFPSRaritySettings.generated.h"

class UNiagaraSystem;

USTRUCT()
struct FLastFPSRarityVisuals
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Rarity", meta=(AllowedClasses="/Script/Niagara.NiagaraSystem"))
	TSoftObjectPtr<UNiagaraSystem> SpawnFX;
};

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LastFPS Rarity"))
class LASTFPS_API ULastFPSRaritySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="Rarity")
	TMap<ELastFPSItemRarity, FLastFPSRarityVisuals> Visuals;

	static const ULastFPSRaritySettings* Get() { return GetDefault<ULastFPSRaritySettings>(); }

	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};
