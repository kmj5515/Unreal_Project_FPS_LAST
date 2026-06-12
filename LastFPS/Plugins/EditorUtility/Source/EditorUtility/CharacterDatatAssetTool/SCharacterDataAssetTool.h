#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

struct FAssetData;
class UDataTable;

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
	void RefreshRowNames();
	TSharedRef<SWidget> GenerateRowNameWidget(TSharedPtr<FName> RowName) const;
	FText GetSelectedRowNameText() const;
	FReply OpenOutputRootPicker();
	void HandleOutputRootSelected(const FString& SelectedPath);
	FReply GenerateCharacterDefinition();
	FText GetOutputRootText() const;
	bool CanGenerate() const;

	UObject* LoadSoftObject(const FSoftObjectPath& Path) const;
	void ApplyRowToDefinition(class ULastFPSCharacterDefinition* Definition, const struct FLastFPSCharacterMasterData& Row, FName RowName) const;

	TWeakObjectPtr<UDataTable> MasterTable;
	FString MasterTableObjectPath;
	FString SelectedRowNameString;
	TArray<TSharedPtr<FName>> RowNames;
	TSharedPtr<FName> SelectedRowName;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> RowNameComboBox;
	FString OutputRoot = TEXT("/Game/Data/Characters");
};
