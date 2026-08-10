#include "DataTableImportTool/SLastFPSDataTableImportTool.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "Engine/DataTable.h"
#include "Framework/Application/SlateApplication.h"
#include "IContentBrowserSingleton.h"
#include "IDesktopPlatform.h"
#include "Internationalization/StringTable.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Settings/LastFPSDataTableImportSettings.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSDataTableImportTool"

namespace
{
	FSlateColor GetSheetStateColor(const ELastFPSDataTableImportSheetState State)
	{
		switch (State)
		{
		case ELastFPSDataTableImportSheetState::Ready:
			return FSlateColor(FLinearColor(0.45f, 0.85f, 0.55f));
		case ELastFPSDataTableImportSheetState::Unregistered:
		case ELastFPSDataTableImportSheetState::InvalidMapping:
			return FSlateColor(FLinearColor(1.f, 0.65f, 0.15f));
		default:
			return FSlateColor::UseSubduedForeground();
		}
	}
}

void SLastFPSDataTableImportTool::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("ExcelDirectory", "엑셀 폴더"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SAssignNew(DirectoryTextBox, SEditableTextBox)
					.Text(FText::FromString(ULastFPSDataTableImportSettings::Get()->ExcelDirectory.Path))
					.OnTextCommitted(this, &SLastFPSDataTableImportTool::DirectoryCommitted)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Browse", "찾아보기"))
					.OnClicked(this, &SLastFPSDataTableImportTool::BrowseDirectoryClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("Refresh", "새로고침"))
					.OnClicked(this, &SLastFPSDataTableImportTool::RefreshClicked)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(210.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(WorkbookListBox, SVerticalBox)
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(WorkbookHeaderText, STextBlock)
						.Text(this, &SLastFPSDataTableImportTool::GetWorkbookHeaderText)
						.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					[
						SNew(SBorder)
						.Padding(6.f)
						.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(SheetListBox, SVerticalBox)
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ImportSelected", "선택 항목 임포트"))
							.OnClicked(this, &SLastFPSDataTableImportTool::ImportSelectedClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("ImportWorkbook", "워크북 전체 임포트"))
							.OnClicked(this, &SLastFPSDataTableImportTool::ImportWorkbookClicked)
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("Ready", "로그: 준비됨"))
				.AutoWrapText(true)
			]
		]
	];

	Refresh();
}

void SLastFPSDataTableImportTool::Refresh()
{
	FText Error;
	// 재스캔으로 배열 순서가 바뀌어도 사용자가 보던 워크북을 이름으로 복원해 인덱스 오선택을 피한다.
	const FName PreviousWorkbook = Workbooks.IsValidIndex(SelectedWorkbookIndex)
		? Workbooks[SelectedWorkbookIndex].WorkbookName
		: NAME_None;
	if (!FLastFPSDataTableImportService::ScanWorkbooks(Workbooks, Error))
	{
		SelectedWorkbookIndex = INDEX_NONE;
		SetStatus(FText::Format(LOCTEXT("ScanFailed", "로그: 스캔 실패 - {0}"), Error));
	}
	else
	{
		SelectedWorkbookIndex = Workbooks.IndexOfByPredicate([PreviousWorkbook](const FLastFPSDataTableImportWorkbookInfo& Workbook)
		{
			return Workbook.WorkbookName == PreviousWorkbook;
		});
		if (SelectedWorkbookIndex == INDEX_NONE && !Workbooks.IsEmpty())
		{
			SelectedWorkbookIndex = 0;
		}
		SetStatus(Error.IsEmpty()
			? FText::Format(LOCTEXT("ScanComplete", "로그: 워크북 {0}개를 찾았습니다."), FText::AsNumber(Workbooks.Num()))
			: FText::Format(LOCTEXT("ScanWarning", "로그: {0}"), Error));
	}

	RebuildWorkbookList();
	SelectWorkbook(SelectedWorkbookIndex);
}

void SLastFPSDataTableImportTool::RebuildWorkbookList()
{
	if (!WorkbookListBox)
	{
		return;
	}
	WorkbookListBox->ClearChildren();
	for (int32 Index = 0; Index < Workbooks.Num(); ++Index)
	{
		WorkbookListBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)[BuildWorkbookRow(Index)];
	}
}

void SLastFPSDataTableImportTool::RebuildSheetList()
{
	if (!SheetListBox)
	{
		return;
	}
	SheetListBox->ClearChildren();
	if (!Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		SheetListBox->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoWorkbook", "워크북이 없습니다."))];
		return;
	}
	for (const FLastFPSDataTableImportSheetInfo& Sheet : Workbooks[SelectedWorkbookIndex].Sheets)
	{
		SheetListBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)[BuildSheetRow(Sheet)];
	}
}

void SLastFPSDataTableImportTool::SelectWorkbook(const int32 WorkbookIndex)
{
	SelectedWorkbookIndex = Workbooks.IsValidIndex(WorkbookIndex) ? WorkbookIndex : INDEX_NONE;
	SelectedSheets.Reset();
	// 제외·미등록·잘못된 매핑은 선택 단계부터 차단해 서비스에 모호한 임포트 요청이 전달되지 않게 한다.
	if (Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		for (const FLastFPSDataTableImportSheetInfo& Sheet : Workbooks[SelectedWorkbookIndex].Sheets)
		{
			if (Sheet.State == ELastFPSDataTableImportSheetState::Ready)
			{
				SelectedSheets.Add(Sheet.SheetName);
			}
		}
	}
	RebuildWorkbookList();
	RebuildSheetList();
}

TSharedRef<SWidget> SLastFPSDataTableImportTool::BuildWorkbookRow(const int32 WorkbookIndex)
{
	const FLastFPSDataTableImportWorkbookInfo& Workbook = Workbooks[WorkbookIndex];
	FString Badge;
	if (Workbook.bHasUnregisteredSheets)
	{
		Badge = TEXT("  ⚠");
	}
	else if (Workbook.bModifiedSinceImport)
	{
		Badge = TEXT("  ●");
	}
	return SNew(SButton)
		.ButtonColorAndOpacity(WorkbookIndex == SelectedWorkbookIndex ? FLinearColor(0.20f, 0.35f, 0.55f) : FLinearColor::White)
		.Text(FText::FromString(FPaths::GetBaseFilename(Workbook.WorkbookName.ToString()) + Badge))
		.ToolTipText(FText::FromString(Workbook.WorkbookPath))
		.OnClicked_Lambda([this, WorkbookIndex]()
		{
			SelectWorkbook(WorkbookIndex);
			return FReply::Handled();
		});
}

TSharedRef<SWidget> SLastFPSDataTableImportTool::BuildSheetRow(const FLastFPSDataTableImportSheetInfo& Sheet)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (Sheet.State == ELastFPSDataTableImportSheetState::Ready)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked(this, &SLastFPSDataTableImportTool::GetSheetCheckState, Sheet.SheetName)
			.OnCheckStateChanged(this, &SLastFPSDataTableImportTool::SheetCheckChanged, Sheet.SheetName)
		];
	}
	else
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(STextBlock).Text(Sheet.State == ELastFPSDataTableImportSheetState::Excluded ? FText::FromString(TEXT("—")) : FText::FromString(TEXT("⚠")))
		];
	}

	const FString TargetLabel = Sheet.TargetAssetPath.IsNull()
		? Sheet.StateMessage.ToString()
		: FString::Printf(TEXT("→ %s"), *Sheet.TargetAssetPath.GetAssetName());
	Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Sheet.SheetName.ToString() + TEXT("  ") + TargetLabel))
		.ColorAndOpacity(GetSheetStateColor(Sheet.State))
		.ToolTipText(Sheet.TargetAssetPath.IsNull() ? Sheet.StateMessage : FText::FromString(Sheet.TargetAssetPath.ToString()))
	];

	if (Sheet.State == ELastFPSDataTableImportSheetState::Unregistered)
	{
		Row->AddSlot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddToRegistry", "레지스트리에 추가"))
			.OnClicked(this, &SLastFPSDataTableImportTool::AddMappingClicked, Sheet.SheetName)
		];
	}
	return SNew(SBorder).Padding(5.f).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))[Row];
}

FReply SLastFPSDataTableImportTool::BrowseDirectoryClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		SetStatus(LOCTEXT("NoDesktopPlatform", "로그: 폴더 선택 대화상자를 열 수 없습니다."));
		return FReply::Handled();
	}

	FString ChosenDirectory;
	const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	if (DesktopPlatform->OpenDirectoryDialog(
		ParentWindowHandle,
		LOCTEXT("ChooseExcelDirectory", "Excel 원본 폴더 선택").ToString(),
		FLastFPSDataTableImportService::GetExcelDirectory(),
		ChosenDirectory))
	{
		ULastFPSDataTableImportSettings* Settings = GetMutableDefault<ULastFPSDataTableImportSettings>();
		FText Error;
		if (!Settings->SetExcelDirectoryFromAbsolutePath(ChosenDirectory, Error))
		{
			SetStatus(FText::Format(LOCTEXT("DirectoryRejected", "로그: {0}"), Error));
		}
		else
		{
			DirectoryTextBox->SetText(FText::FromString(Settings->ExcelDirectory.Path));
			Refresh();
		}
	}
	return FReply::Handled();
}

FReply SLastFPSDataTableImportTool::RefreshClicked()
{
	Refresh();
	return FReply::Handled();
}

FReply SLastFPSDataTableImportTool::ImportSelectedClicked()
{
	if (!Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		SetStatus(LOCTEXT("NoSelectedWorkbook", "로그: 선택된 워크북이 없습니다."));
		return FReply::Handled();
	}
	TArray<FLastFPSDataTableImportLogEntry> Entries;
	FLastFPSDataTableImportService::ImportSheets(Workbooks[SelectedWorkbookIndex], SelectedSheets, Entries);
	Refresh();
	AppendImportLog(Entries);
	return FReply::Handled();
}

FReply SLastFPSDataTableImportTool::ImportWorkbookClicked()
{
	if (!Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		SetStatus(LOCTEXT("NoWorkbookForFullImport", "로그: 선택된 워크북이 없습니다."));
		return FReply::Handled();
	}
	TSet<FName> AllReadySheets;
	for (const FLastFPSDataTableImportSheetInfo& Sheet : Workbooks[SelectedWorkbookIndex].Sheets)
	{
		if (Sheet.State == ELastFPSDataTableImportSheetState::Ready)
		{
			AllReadySheets.Add(Sheet.SheetName);
		}
	}
	TArray<FLastFPSDataTableImportLogEntry> Entries;
	FLastFPSDataTableImportService::ImportSheets(Workbooks[SelectedWorkbookIndex], AllReadySheets, Entries);
	Refresh();
	AppendImportLog(Entries);
	return FReply::Handled();
}

FReply SLastFPSDataTableImportTool::AddMappingClicked(const FName SheetName)
{
	if (!Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		return FReply::Handled();
	}

	FOpenAssetDialogConfig DialogConfig;
	DialogConfig.DialogTitleOverride = LOCTEXT("ChooseTargetAsset", "DataTable 또는 StringTable 선택");
	DialogConfig.DefaultPath = TEXT("/Game/Data/Tables");
	DialogConfig.bAllowMultipleSelection = false;
	DialogConfig.AssetClassNames.Add(UDataTable::StaticClass()->GetClassPathName());
	DialogConfig.AssetClassNames.Add(UStringTable::StaticClass()->GetClassPathName());
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	const TArray<FAssetData> SelectedAssets = ContentBrowserModule.Get().CreateModalOpenAssetDialog(DialogConfig);
	if (SelectedAssets.Num() == 1)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			SetStatus(LOCTEXT("NoDesktopPlatformForCsv", "로그: CSV 파일명 선택 대화상자를 열 수 없습니다."));
			return FReply::Handled();
		}

		TArray<FString> SelectedCsvFiles;
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		if (!DesktopPlatform->SaveFileDialog(
			ParentWindowHandle,
			LOCTEXT("ChooseCsvFileName", "매핑에 사용할 CSV 파일명 지정").ToString(),
			FLastFPSDataTableImportService::GetExcelDirectory(),
			TEXT(""),
			TEXT("CSV files (*.csv)|*.csv"),
			EFileDialogFlags::None,
			SelectedCsvFiles)
			|| SelectedCsvFiles.Num() != 1)
		{
			return FReply::Handled();
		}

		FString CsvPath = FPaths::ConvertRelativePathToFull(SelectedCsvFiles[0]);
		if (FPaths::GetExtension(CsvPath).IsEmpty())
		{
			CsvPath += TEXT(".csv");
		}
		FPaths::NormalizeFilename(CsvPath);
		FString CsvDirectory = FPaths::GetPath(CsvPath);
		FPaths::NormalizeDirectoryName(CsvDirectory);
		FString ExcelDirectory = FPaths::ConvertRelativePathToFull(FLastFPSDataTableImportService::GetExcelDirectory());
		FPaths::NormalizeDirectoryName(ExcelDirectory);
		if (!FPaths::IsSamePath(CsvDirectory, ExcelDirectory)
			|| !FPaths::GetExtension(CsvPath).Equals(TEXT("csv"), ESearchCase::IgnoreCase))
		{
			SetStatus(FText::Format(
				LOCTEXT("CsvFileOutsideExcelDirectory", "로그: CSV 파일은 Excel 폴더 바로 아래의 .csv 파일로 지정해야 합니다. 선택={0}"),
				FText::FromString(CsvPath)));
			return FReply::Handled();
		}

		FText Error;
		if (FLastFPSDataTableImportService::AddRegistryMapping(
			Workbooks[SelectedWorkbookIndex].WorkbookName,
			SheetName,
			FPaths::GetCleanFilename(CsvPath),
			SelectedAssets[0].GetSoftObjectPath(),
			Error))
		{
			Refresh();
		}
		else
		{
			SetStatus(FText::Format(LOCTEXT("MappingAddFailed", "로그: {0}"), Error));
		}
	}
	return FReply::Handled();
}

void SLastFPSDataTableImportTool::DirectoryCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnCleared)
	{
		return;
	}
	FString Candidate = NewText.ToString();
	if (FPaths::IsRelative(Candidate))
	{
		Candidate = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), Candidate));
	}
	ULastFPSDataTableImportSettings* Settings = GetMutableDefault<ULastFPSDataTableImportSettings>();
	FText Error;
	if (!Settings->SetExcelDirectoryFromAbsolutePath(Candidate, Error))
	{
		SetStatus(FText::Format(LOCTEXT("DirectoryCommitFailed", "로그: {0}"), Error));
		DirectoryTextBox->SetText(FText::FromString(Settings->ExcelDirectory.Path));
		return;
	}
	DirectoryTextBox->SetText(FText::FromString(Settings->ExcelDirectory.Path));
	Refresh();
}

void SLastFPSDataTableImportTool::SheetCheckChanged(const ECheckBoxState NewState, const FName SheetName)
{
	if (NewState == ECheckBoxState::Checked)
	{
		SelectedSheets.Add(SheetName);
	}
	else
	{
		SelectedSheets.Remove(SheetName);
	}
}

ECheckBoxState SLastFPSDataTableImportTool::GetSheetCheckState(const FName SheetName) const
{
	return SelectedSheets.Contains(SheetName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SLastFPSDataTableImportTool::GetWorkbookHeaderText() const
{
	if (!Workbooks.IsValidIndex(SelectedWorkbookIndex))
	{
		return LOCTEXT("NoWorkbookHeader", "워크북 없음");
	}
	const FLastFPSDataTableImportWorkbookInfo& Workbook = Workbooks[SelectedWorkbookIndex];
	return FText::Format(
		LOCTEXT("WorkbookHeader", "{0}    수정 {1}"),
		FText::FromName(Workbook.WorkbookName),
		FText::AsDateTime(Workbook.ModifiedTime));
}

void SLastFPSDataTableImportTool::SetStatus(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

void SLastFPSDataTableImportTool::AppendImportLog(const TArray<FLastFPSDataTableImportLogEntry>& Entries)
{
	if (Entries.IsEmpty())
	{
		SetStatus(LOCTEXT("NoImportEntries", "로그: 임포트할 항목이 없습니다."));
		return;
	}
	TArray<FString> Lines;
	int32 SuccessCount = 0;
	for (const FLastFPSDataTableImportLogEntry& Entry : Entries)
	{
		SuccessCount += Entry.bSucceeded ? 1 : 0;
		Lines.Add(FString::Printf(
			TEXT("%s %s: %s"),
			Entry.bSucceeded ? TEXT("성공") : TEXT("실패"),
			*Entry.SheetName.ToString(),
			*Entry.Message.ToString()));
	}
	Lines.Insert(FString::Printf(TEXT("로그: %d개 중 %d개 성공"), Entries.Num(), SuccessCount), 0);
	SetStatus(FText::FromString(FString::Join(Lines, TEXT("\n"))));
}

#undef LOCTEXT_NAMESPACE
