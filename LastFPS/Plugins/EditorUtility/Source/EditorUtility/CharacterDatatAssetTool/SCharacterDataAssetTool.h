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
	FString GetMasterTableObjectPath() const;
	void SetLocomotionAnimationSet(const FAssetData& AssetData);
	FString GetLocomotionAnimationSetObjectPath() const;

	void RefreshRowNames();
	TSharedRef<SWidget> GenerateRowNameWidget(TSharedPtr<FName> RowName) const;

	/** 콤보에서 고른 라벨(Hero/Enemy)을 실제 생성할 Definition 서브클래스로 변환. */
	UClass* ResolveDefinitionClass() const;
	/** 선택된 행의 CharacterType에 맞춰 생성 타입 콤보의 기본값을 잡아준다. */
	void UpdateDefaultDefinitionTypeFromRow();

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

	// 생성할 Definition 타입 선택(Hero/Enemy)
	TArray<TSharedPtr<FString>> DefinitionTypeOptions;
	TSharedPtr<FString> SelectedDefinitionType;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> DefinitionTypeComboBox;
	FString DefinitionOutputRoot = TEXT("/Game/Data/Characters");
	FString StatOutputRoot = TEXT("/Game/Data/Characters");
	FString AbilitySetOutputRoot = TEXT("/Game/Data/Characters");
	TWeakObjectPtr<ULastFPSLocomotionAnimationSet> LocomotionAnimationSet;
	FString LocomotionAnimationSetObjectPath;
	FString LocomotionAnimationSourceRoot = TEXT("/Game/Characters/Player/Animations/MotionMatching");
	FString LocomotionAnimationNameFilter = TEXT("Unarmed");
	FString LocomotionAnimationPrefixFilter = TEXT("MF_");
};
