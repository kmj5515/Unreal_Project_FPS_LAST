#pragma once

#include "DataTableImportTool/LastFPSDataTableImportService.h"
#include "Engine/DataTable.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class IStructureDetailsView;
class SEditableTextBox;
template<typename OptionType> class SComboBox;
class STextBlock;
class SVerticalBox;
class FStructOnScope;

class SLastFPSDataTableImportTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLastFPSDataTableImportTool) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// 위젯은 표시와 사용자 선택만 소유하고, 파일·에셋 변경 책임은 서비스에 위임한다.
	void Refresh();
	void RebuildWorkbookList();
	void RebuildSheetList();
	void SelectWorkbook(int32 WorkbookIndex);
	TSharedRef<SWidget> BuildWorkbookRow(int32 WorkbookIndex);
	TSharedRef<SWidget> BuildSheetRow(const FLastFPSDataTableImportSheetInfo& Sheet);
	FReply BrowseDirectoryClicked();
	FReply RefreshClicked();
	FReply ImportSelectedClicked();
	FReply ImportWorkbookClicked();
	FReply AddMappingClicked(FName SheetName);
	FReply PreviewSheetClicked(FName SheetName);
	FReply ShowCurrentDataClicked();
	FReply ShowExcelPreviewClicked();
	void DirectoryCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void SheetCheckChanged(ECheckBoxState NewState, FName SheetName);
	ECheckBoxState GetSheetCheckState(FName SheetName) const;
	FText GetWorkbookHeaderText() const;
	FText GetPreviewHeaderText() const;
	TSharedRef<SWidget> GeneratePreviewRowWidget(TSharedPtr<FName> RowName) const;
	void PreviewRowSelectionChanged(TSharedPtr<FName> RowName, ESelectInfo::Type SelectInfo);
	void RebuildDataTablePreview(bool bUseExcelValues);
	void RefreshPreviewRows();
	void ClearDataTablePreview(const FText& Message);
	void SetStatus(const FText& Message);
	void AppendImportLog(const TArray<FLastFPSDataTableImportLogEntry>& Entries);

	TArray<FLastFPSDataTableImportWorkbookInfo> Workbooks;
	// 선택 상태는 새 스캔 결과에 섞이지 않는 일시적인 UI 상태이므로 레지스트리나 설정에 저장하지 않는다.
	TSet<FName> SelectedSheets;
	int32 SelectedWorkbookIndex = INDEX_NONE;
	TSharedPtr<SEditableTextBox> DirectoryTextBox;
	TSharedPtr<SVerticalBox> WorkbookListBox;
	TSharedPtr<SVerticalBox> SheetListBox;
	TSharedPtr<STextBlock> WorkbookHeaderText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<STextBlock> PreviewHeaderText;
	TSharedPtr<STextBlock> PreviewMessageText;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> PreviewRowComboBox;
	TSharedPtr<IStructureDetailsView> PreviewDetailsView;
	TSharedPtr<FStructOnScope> PreviewRowData;
	TArray<TSharedPtr<FName>> PreviewRowNames;
	TSharedPtr<FName> SelectedPreviewRow;
	TStrongObjectPtr<UDataTable> PreviewDataTable;
	FName PreviewSheetName;
	bool bShowingExcelValues = false;
};
