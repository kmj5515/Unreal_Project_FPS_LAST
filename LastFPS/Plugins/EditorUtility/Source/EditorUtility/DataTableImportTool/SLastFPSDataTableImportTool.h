#pragma once

#include "DataTableImportTool/LastFPSDataTableImportService.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;
class SVerticalBox;

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
	void DirectoryCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void SheetCheckChanged(ECheckBoxState NewState, FName SheetName);
	ECheckBoxState GetSheetCheckState(FName SheetName) const;
	FText GetWorkbookHeaderText() const;
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
};
