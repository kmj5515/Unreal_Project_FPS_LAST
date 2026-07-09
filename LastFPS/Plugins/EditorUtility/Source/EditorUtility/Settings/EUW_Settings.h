#pragma once

#include "CoreMinimal.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"
#include "Engine/DeveloperSettings.h"
#include "EUW_Settings.generated.h"

class UDataTable;
class UEditorUtilityWidgetBlueprint;
class UWorld;

UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="LastFPS Editor Tools"))
class EDITORUTILITY_API UEUW_Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEUW_Settings();

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

	static const UEUW_Settings* Get() { return GetDefault<UEUW_Settings>(); }
};
