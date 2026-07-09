#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "Engine/DeveloperSettings.h"
#include "MW_Settings.generated.h"

class UDataTable;
class UEditorUtilityWidgetBlueprint;
class UWorld;

UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS Editor Tools"))
class EDITORUTILITY_API UMW_Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMW_Settings();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif

	UPROPERTY(Config, EditAnywhere, Category="Level Selection Tool", meta=(AllowedClasses="/Script/Blutility.EditorUtilityWidgetBlueprint"))
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> LevelSelectionTool;

	UPROPERTY(Config, EditAnywhere, Category="Play Tools")
	TSoftObjectPtr<UWorld> ForcedPlayStartMap;

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	TSoftObjectPtr<UDataTable> CharacterMasterTable;

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterDefinitionOutputRoot = TEXT("/Game/Data/Characters");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterStatOutputRoot = TEXT("/Game/Data/Characters");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterAbilitySetOutputRoot = TEXT("/Game/Data/Characters");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	TSoftObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionAnimationSet;

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString LocomotionAnimationSourceRoot = TEXT("/Game/Characters/Player/Animations/MotionMatching");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString LocomotionAnimationNameFilter = TEXT("Unarmed");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString LocomotionAnimationPrefixFilter = TEXT("MF_");

	static const UMW_Settings* Get() { return GetDefault<UMW_Settings>(); }
};
