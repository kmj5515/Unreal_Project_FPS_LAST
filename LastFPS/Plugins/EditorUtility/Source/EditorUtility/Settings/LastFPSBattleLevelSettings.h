#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "LastFPSBattleLevelSettings.generated.h"

class UWorld;

UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS Battle Levels"))
class EDITORUTILITY_API ULastFPSBattleLevelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULastFPSBattleLevelSettings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif

	UPROPERTY(Config, EditAnywhere, Category="Battle Levels", meta=(ContentDir))
	FDirectoryPath BattleLevelRootPath;

	UPROPERTY(Config, EditAnywhere, Category="Battle Levels")
	TSoftObjectPtr<UWorld> DefaultBattlePlayMap;

	UPROPERTY(Config, EditAnywhere, Category="Validation")
	bool bRequirePlayerStart = true;

	UPROPERTY(Config, EditAnywhere, Category="Validation")
	TArray<FName> RequiredActorTags;

	static const ULastFPSBattleLevelSettings* Get() { return GetDefault<ULastFPSBattleLevelSettings>(); }
};
