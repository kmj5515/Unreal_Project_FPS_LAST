#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;
class UDataTable;
class ULastFPSLocomotionAnimationSet;

class SCharacterDataAssetTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterDataAssetTool) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void LoadSettings();
	void SaveSettings() const;
	void SetMasterTable(const FAssetData& AssetData);
	FString GetMasterTableObjectPath() const;
	void SetLocomotionAnimationSet(const FAssetData& AssetData);
	FString GetLocomotionAnimationSetObjectPath() const;

	void RefreshRowNames();
	TSharedRef<SWidget> GenerateRowNameWidget(TSharedPtr<FName> RowName) const;

	FText GetSelectedRowNameText() const;
	FReply OpenFolderPicker(FString SCharacterDataAssetTool::*OutputRootMember);

	FReply GenerateCharacterDefinition();
	bool CanGenerate() const;

	TSharedRef<SWidget> DrawFolderPickerSection(
		const FText& LabelText,
		FString SCharacterDataAssetTool::*OutputRootMember,
		const FText& GenerateButtonText,
		FReply (SCharacterDataAssetTool::*OnGenerateClicked)(),
		bool bRequiresCharacterRow = true);

	FReply GenerateCharacterStat();

	FReply GenerateCharacterAbilitySet();
	FReply AutoFillLocomotionAnimationSet();

	UObject* LoadSoftObject(const FSoftObjectPath& Path) const;
	void ApplyRowToDefinition(class ULastFPSCharacterDefinition* Definition, const struct FLastFPSCharacterMasterData& Row, FName RowName) const;

	TWeakObjectPtr<UDataTable> MasterTable;
	FString MasterTableObjectPath;
	FString SelectedRowNameString;
	TArray<TSharedPtr<FName>> RowNames;
	TSharedPtr<FName> SelectedRowName;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> RowNameComboBox;
	FString DefinitionOutputRoot = TEXT("/Game/Data/Characters");
	FString StatOutputRoot = TEXT("/Game/Data/Characters");
	FString AbilitySetOutputRoot = TEXT("/Game/Data/Characters");
	TWeakObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionAnimationSet;
	FString LocomotionAnimationSetObjectPath;
	FString LocomotionAnimationSourceRoot = TEXT("/Game/Characters/Player/Animations/MotionMatching");
	FString LocomotionAnimationNameFilter = TEXT("Unarmed");
	FString LocomotionAnimationPrefixFilter = TEXT("MF_");
};
