#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EUW_Settings.generated.h"

class UDataTable;
class UEditorUtilityWidgetBlueprint;

UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="LastFPS Editor Tools"))
class EDITORUTILITY_API UEUW_Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEUW_Settings();

	UPROPERTY(Config, EditAnywhere, Category="Level Selection Tool", meta=(AllowedClasses="/Script/Blutility.EditorUtilityWidgetBlueprint"))
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> LevelSelectionTool;

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	TSoftObjectPtr<UDataTable> CharacterMasterTable;

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterDefinitionOutputRoot = TEXT("/Game/Data/Characters");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterStatOutputRoot = TEXT("/Game/Data/Characters");

	UPROPERTY(Config, EditAnywhere, Category="Character Data Asset Tool")
	FString CharacterAbilitySetOutputRoot = TEXT("/Game/Data/Characters");

	static const UEUW_Settings* Get() { return GetDefault<UEUW_Settings>(); }
};
