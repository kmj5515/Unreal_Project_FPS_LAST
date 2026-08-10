#include "DataTableImportTool/LastFPSDataTableImportService.h"

#include "DataTableImportTool/LastFPSDataTableImportRegistry.h"
#include "Editor.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Internationalization/StringTable.h"
#include "IPythonScriptPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "Localization/LastFPSStringTableSyncLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LastFPSDataTableImportSettings.h"
#include "Subsystems/EditorAssetSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSDataTableImport, Log, All);

#define LOCTEXT_NAMESPACE "LastFPSDataTableImportService"

namespace
{
	// 레지스트리는 게임 콘텐츠가 아니라 에디터 전용 규칙이므로 패키징 대상인 /Game 대신 플러그인의 /EditorUtility/ 마운트에 둔다.
	const TCHAR* RegistryObjectPath = TEXT("/EditorUtility/Editor/DataTableImport/DA_DataTableImportRegistry.DA_DataTableImportRegistry");

	FString QuotePythonArgument(FString Value)
	{
		Value.ReplaceInline(TEXT("\\"), TEXT("/"));
		Value.ReplaceInline(TEXT("\""), TEXT("\\\""));
		return FString::Printf(TEXT("\"%s\""), *Value);
	}

	bool ExecutePython(const TArray<FString>& Arguments, FText& OutError)
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EditorUtility"));
		if (!Plugin.IsValid())
		{
			OutError = LOCTEXT("PluginNotFound", "EditorUtility 플러그인 경로를 찾을 수 없습니다.");
			return false;
		}

		IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
		if (!PythonPlugin)
		{
			PythonPlugin = FModuleManager::LoadModulePtr<IPythonScriptPlugin>(TEXT("PythonScriptPlugin"));
		}
		if (!PythonPlugin || !PythonPlugin->IsPythonInitialized())
		{
			OutError = LOCTEXT("PythonUnavailable", "PythonScriptPlugin이 초기화되지 않았습니다.");
			return false;
		}

		const FString ScriptPath = FPaths::Combine(
			Plugin->GetBaseDir(), TEXT("Content/Python/import_datatables.py"));
		if (!IFileManager::Get().FileExists(*ScriptPath))
		{
			OutError = FText::Format(
				LOCTEXT("PythonScriptMissing", "Python 변환 스크립트를 찾을 수 없습니다: {0}"),
				FText::FromString(ScriptPath));
			return false;
		}

		FString Command = QuotePythonArgument(ScriptPath);
		for (const FString& Argument : Arguments)
		{
			Command += TEXT(" ") + QuotePythonArgument(Argument);
		}

		if (!PythonPlugin->ExecPythonCommand(*Command))
		{
			OutError = FText::Format(
				LOCTEXT("PythonExecutionFailed", "Python 변환에 실패했습니다. 엑셀 파일 잠금과 Output Log를 확인하세요. 명령={0}"),
				FText::FromString(Command));
			return false;
		}
		return true;
	}

	bool ReadSheetNames(const FString& WorkbookPath, TArray<FString>& OutSheetNames, FText& OutError)
	{
		const FString ScanDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DataTableImport/Scan"));
		IFileManager::Get().MakeDirectory(*ScanDirectory, true);
		const FString OutputPath = FPaths::Combine(
			ScanDirectory,
			FString::Printf(TEXT("%08x.json"), GetTypeHash(WorkbookPath)));

		const TArray<FString> Arguments = {
			TEXT("--project-dir"), FPaths::ProjectDir(),
			TEXT("scan"),
			TEXT("--workbook"), WorkbookPath,
			TEXT("--output"), OutputPath
		};
		if (!ExecutePython(Arguments, OutError))
		{
			return false;
		}

		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *OutputPath))
		{
			OutError = FText::Format(
				LOCTEXT("ScanResultMissing", "워크북 스캔 결과를 읽을 수 없습니다: {0}"),
				FText::FromString(OutputPath));
			return false;
		}

		TSharedPtr<FJsonObject> RootObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
		{
			OutError = FText::Format(
				LOCTEXT("ScanResultInvalid", "워크북 스캔 결과 JSON이 올바르지 않습니다: {0}"),
				FText::FromString(OutputPath));
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* SheetValues = nullptr;
		if (!RootObject->TryGetArrayField(TEXT("sheets"), SheetValues) || !SheetValues)
		{
			OutError = FText::Format(
				LOCTEXT("ScanSheetsMissing", "워크북 스캔 결과에 sheets 배열이 없습니다: {0}"),
				FText::FromString(WorkbookPath));
			return false;
		}

		OutSheetNames.Reset(SheetValues->Num());
		for (const TSharedPtr<FJsonValue>& SheetValue : *SheetValues)
		{
			FString SheetName;
			if (SheetValue.IsValid() && SheetValue->TryGetString(SheetName))
			{
				OutSheetNames.Add(MoveTemp(SheetName));
			}
		}
		return true;
	}

	bool IsExcludedSheet(const FString& SheetName, FText& OutReason)
	{
		// '_' 시트는 계산·참조·문서 등 임포트 계약 밖의 보조 자료를 한 규칙으로 격리해 콘텐츠 시트로 오인하지 않게 한다.
		if (!SheetName.StartsWith(TEXT("_")))
		{
			return false;
		}

		if (SheetName.StartsWith(TEXT("_Ref_")))
		{
			OutReason = LOCTEXT("ReferenceSheet", "제외 (참조 시트)");
		}
		else if (SheetName.StartsWith(TEXT("_Calc_")))
		{
			OutReason = LOCTEXT("CalculationSheet", "제외 (계산 시트)");
		}
		else if (SheetName.StartsWith(TEXT("_Draft_")))
		{
			OutReason = LOCTEXT("DraftSheet", "제외 (작업 중 시트)");
		}
		else if (SheetName.StartsWith(TEXT("_Note")) || SheetName.StartsWith(TEXT("_README")))
		{
			OutReason = LOCTEXT("NoteSheet", "제외 (문서 시트)");
		}
		else
		{
			OutReason = LOCTEXT("PrivateSheet", "제외 (_ 접두사 시트)");
		}
		return true;
	}

	bool ConvertSheet(
		const FLastFPSDataTableImportWorkbookInfo& Workbook,
		const FLastFPSDataTableImportMapping& Mapping,
		const bool bLocalize,
		FString& OutGeneratedCsvPath,
		FString& OutCleanCsvPath,
		FText& OutError)
	{
		const FString ExcelDirectory = FLastFPSDataTableImportService::GetExcelDirectory();
		if (!FLastFPSDataTableImportMapping::IsValidCsvFileName(Mapping.CsvFileName))
		{
			OutError = FText::Format(
				LOCTEXT("InvalidCsvFileName", "CSV 파일명이 비어 있거나 올바르지 않아 변환을 거부했습니다. 워크북={0}, 시트={1}, 파일명={2}"),
				FText::FromName(Workbook.WorkbookName),
				FText::FromName(Mapping.SheetName),
				FText::FromString(Mapping.CsvFileName));
			UE_LOG(LogLastFPSDataTableImport, Warning, TEXT("%s"), *OutError.ToString());
			return false;
		}

		// 커밋용 CSV에는 자동 생성 안내 행을 남겨 직접 편집을 막지만, Unreal 임포터에는 스키마 행만 허용되므로 안내 행이 없는 clean CSV를 별도로 전달한다.
		OutGeneratedCsvPath = FPaths::Combine(ExcelDirectory, Mapping.CsvFileName);
		if (IFileManager::Get().FileExists(*OutGeneratedCsvPath))
		{
			const FDateTime CsvModifiedTime = IFileManager::Get().GetTimeStamp(*OutGeneratedCsvPath);
			const FDateTime WorkbookModifiedTime = IFileManager::Get().GetTimeStamp(*Workbook.WorkbookPath);
			if (CsvModifiedTime > WorkbookModifiedTime)
			{
				OutError = FText::Format(
					LOCTEXT("CsvNewerThanWorkbook", "기존 CSV가 워크북보다 최신이므로 덮어쓰기를 거부했습니다. CSV={0}, 워크북={1}"),
					FText::FromString(OutGeneratedCsvPath),
					FText::FromString(Workbook.WorkbookPath));
				UE_LOG(
					LogLastFPSDataTableImport,
					Warning,
					TEXT("%s CSV수정=%s 워크북수정=%s"),
					*OutError.ToString(),
					*CsvModifiedTime.ToString(),
					*WorkbookModifiedTime.ToString());
				return false;
			}
		}

		const FString CleanDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DataTableImport/Clean"));
		IFileManager::Get().MakeDirectory(*CleanDirectory, true);
		OutCleanCsvPath = FPaths::Combine(
			CleanDirectory,
			Workbook.WorkbookName.ToString() + TEXT("_") + Mapping.SheetName.ToString() + TEXT(".csv"));

		TArray<FString> Arguments = {
			TEXT("--project-dir"), FPaths::ProjectDir(),
			TEXT("convert"),
			TEXT("--workbook"), Workbook.WorkbookPath,
			TEXT("--sheet"), Mapping.SheetName.ToString(),
			TEXT("--output"), OutGeneratedCsvPath,
			TEXT("--clean-output"), OutCleanCsvPath
		};
		if (bLocalize)
		{
			Arguments.Add(TEXT("--localize"));
		}
		return ExecutePython(Arguments, OutError);
	}

	bool SaveImportedAsset(UObject* Asset, FText& OutError)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor
			? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()
			: nullptr;
		if (!AssetSubsystem || !AssetSubsystem->SaveLoadedAsset(Asset, true))
		{
			OutError = FText::Format(
				LOCTEXT("SaveAssetFailed", "갱신한 에셋을 저장하지 못했습니다: {0}"),
				FText::FromString(GetPathNameSafe(Asset)));
			return false;
		}
		return true;
	}

	bool ImportDataTable(UDataTable* DataTable, const FString& CsvPath, FText& OutError)
	{
		FString CsvText;
		if (!FFileHelper::LoadFileToString(CsvText, *CsvPath))
		{
			OutError = FText::Format(
				LOCTEXT("CsvReadFailed", "임포트용 CSV를 읽을 수 없습니다: {0}"),
				FText::FromString(CsvPath));
			return false;
		}

		UDataTable* ValidationCopy = DuplicateObject<UDataTable>(DataTable, GetTransientPackage());
		if (!ValidationCopy)
		{
			OutError = FText::Format(
				LOCTEXT("ValidationCopyFailed", "DataTable 검증 복사본을 만들지 못했습니다: {0}"),
				FText::FromString(GetPathNameSafe(DataTable)));
			return false;
		}
		const TArray<FString> ValidationProblems = ValidationCopy->CreateTableFromCSVString(CsvText);
		if (!ValidationProblems.IsEmpty())
		{
			OutError = FText::Format(
				LOCTEXT("CsvValidationFailed", "DataTable CSV 검증에 실패했습니다: {0}"),
				FText::FromString(FString::Join(ValidationProblems, TEXT(" | "))));
			return false;
		}

		DataTable->Modify();
		const TArray<FString> ImportProblems = DataTable->CreateTableFromCSVString(CsvText);
		if (!ImportProblems.IsEmpty())
		{
			OutError = FText::Format(
				LOCTEXT("CsvImportFailed", "DataTable 갱신에 실패했습니다: {0}"),
				FText::FromString(FString::Join(ImportProblems, TEXT(" | "))));
			return false;
		}

		DataTable->MarkPackageDirty();
		return true;
	}
}

FString FLastFPSDataTableImportService::GetExcelDirectory()
{
	const ULastFPSDataTableImportSettings* Settings = ULastFPSDataTableImportSettings::Get();
	return Settings ? Settings->ResolveExcelDirectory() : FString();
}

ULastFPSDataTableImportRegistry* FLastFPSDataTableImportService::LoadRegistry(FText& OutError)
{
	ULastFPSDataTableImportRegistry* Registry = LoadObject<ULastFPSDataTableImportRegistry>(nullptr, RegistryObjectPath);
	if (!Registry)
	{
		OutError = FText::Format(
			LOCTEXT("RegistryMissing", "DataTable Import Registry 에셋을 찾을 수 없습니다: {0}"),
			FText::FromString(RegistryObjectPath));
	}
	return Registry;
}

bool FLastFPSDataTableImportService::ScanWorkbooks(
	TArray<FLastFPSDataTableImportWorkbookInfo>& OutWorkbooks,
	FText& OutError)
{
	OutWorkbooks.Reset();
	const FString ExcelDirectory = GetExcelDirectory();
	if (!IFileManager::Get().DirectoryExists(*ExcelDirectory))
	{
		OutError = FText::Format(
			LOCTEXT("ExcelDirectoryMissing", "엑셀 폴더를 찾을 수 없습니다: {0}"),
			FText::FromString(ExcelDirectory));
		return false;
	}

	FText RegistryError;
	ULastFPSDataTableImportRegistry* Registry = LoadRegistry(RegistryError);
	TArray<FString> WorkbookFiles;
	// '~$' 파일은 Excel이 편집 중 만든 잠금용 임시 파일이라 실제 워크북으로 열면 중복 표시나 읽기 실패가 발생할 수 있다.
	IFileManager::Get().FindFiles(WorkbookFiles, *FPaths::Combine(ExcelDirectory, TEXT("*.xlsx")), true, false);
	WorkbookFiles.RemoveAll([](const FString& FileName) { return FileName.StartsWith(TEXT("~$")); });
	WorkbookFiles.Sort();

	for (const FString& WorkbookFile : WorkbookFiles)
	{
		FLastFPSDataTableImportWorkbookInfo& Workbook = OutWorkbooks.AddDefaulted_GetRef();
		Workbook.WorkbookName = FName(*WorkbookFile);
		Workbook.WorkbookPath = FPaths::Combine(ExcelDirectory, WorkbookFile);
		Workbook.ModifiedTime = IFileManager::Get().GetTimeStamp(*Workbook.WorkbookPath);

		TArray<FString> SheetNames;
		FText ScanError;
		if (!ReadSheetNames(Workbook.WorkbookPath, SheetNames, ScanError))
		{
			OutError = ScanError;
			return false;
		}

		for (const FString& SheetNameString : SheetNames)
		{
			FLastFPSDataTableImportSheetInfo& Sheet = Workbook.Sheets.AddDefaulted_GetRef();
			Sheet.SheetName = FName(*SheetNameString);
			if (IsExcludedSheet(SheetNameString, Sheet.StateMessage))
			{
				Sheet.State = ELastFPSDataTableImportSheetState::Excluded;
				continue;
			}

			const FLastFPSDataTableImportMapping* Mapping = Registry
				? Registry->FindMapping(Workbook.WorkbookName, Sheet.SheetName)
				: nullptr;
			if (!Mapping)
			{
				// 미등록 시트를 자동 무시하면 신규 콘텐츠 누락을 발견하기 어려우므로 명시적 '_' 제외와 달리 경고 상태로 남긴다.
				Sheet.State = ELastFPSDataTableImportSheetState::Unregistered;
				Sheet.StateMessage = LOCTEXT(
					"UnregisteredSheet",
					"레지스트리 미등록 - 임포트하려면 레지스트리에 등록, 제외하려면 이름 앞에 _를 붙이세요.");
				Workbook.bHasUnregisteredSheets = true;
				continue;
			}

			Sheet.TargetAssetPath = Mapping->TargetAsset.ToSoftObjectPath();
			Sheet.CsvFileName = Mapping->CsvFileName;
			if (Sheet.TargetAssetPath.IsNull())
			{
				Sheet.State = ELastFPSDataTableImportSheetState::InvalidMapping;
				Sheet.StateMessage = LOCTEXT("InvalidMapping", "레지스트리 대상 에셋이 비어 있습니다.");
				continue;
			}
			if (!FLastFPSDataTableImportMapping::IsValidCsvFileName(Sheet.CsvFileName))
			{
				Sheet.State = ELastFPSDataTableImportSheetState::InvalidMapping;
				Sheet.StateMessage = LOCTEXT("InvalidMappingCsvFileName", "레지스트리 CSV 파일명이 비어 있거나 올바르지 않습니다.");
				continue;
			}

			Sheet.State = ELastFPSDataTableImportSheetState::Ready;
			Sheet.StateMessage = LOCTEXT("ReadySheet", "임포트 가능");
			const FString CsvPath = FPaths::Combine(ExcelDirectory, Sheet.CsvFileName);
			const FDateTime CsvModifiedTime = IFileManager::Get().GetTimeStamp(*CsvPath);
			Workbook.bModifiedSinceImport |= !IFileManager::Get().FileExists(*CsvPath)
				|| Workbook.ModifiedTime > CsvModifiedTime;
		}
	}

	OutError = Registry ? FText::GetEmpty() : RegistryError;
	return true;
}

bool FLastFPSDataTableImportService::ImportSheets(
	const FLastFPSDataTableImportWorkbookInfo& Workbook,
	const TSet<FName>& SelectedSheets,
	TArray<FLastFPSDataTableImportLogEntry>& OutLogEntries)
{
	OutLogEntries.Reset();
	FText RegistryError;
	ULastFPSDataTableImportRegistry* Registry = LoadRegistry(RegistryError);
	if (!Registry)
	{
		FLastFPSDataTableImportLogEntry& LogEntry = OutLogEntries.AddDefaulted_GetRef();
		LogEntry.Message = RegistryError;
		return false;
	}

	bool bAllSucceeded = true;
	for (const FLastFPSDataTableImportSheetInfo& Sheet : Workbook.Sheets)
	{
		if (Sheet.State != ELastFPSDataTableImportSheetState::Ready || !SelectedSheets.Contains(Sheet.SheetName))
		{
			continue;
		}

		FLastFPSDataTableImportLogEntry& LogEntry = OutLogEntries.AddDefaulted_GetRef();
		LogEntry.SheetName = Sheet.SheetName;
		const FLastFPSDataTableImportMapping* Mapping = Registry->FindMapping(Workbook.WorkbookName, Sheet.SheetName);
		if (!Mapping)
		{
			LogEntry.Message = LOCTEXT("MappingDisappeared", "임포트 중 레지스트리 매핑을 찾지 못했습니다.");
			bAllSucceeded = false;
			continue;
		}

		UObject* TargetAsset = Mapping->TargetAsset.LoadSynchronous();
		UDataTable* DataTable = Cast<UDataTable>(TargetAsset);
		UStringTable* StringTable = Cast<UStringTable>(TargetAsset);
		if (!DataTable && !StringTable)
		{
			LogEntry.Message = FText::Format(
				LOCTEXT("UnsupportedTarget", "지원하지 않는 대상 에셋 형식입니다: {0}"),
				FText::FromString(Mapping->TargetAsset.ToString()));
			bAllSucceeded = false;
			continue;
		}

		FString GeneratedCsvPath;
		FString CleanCsvPath;
		FText OperationError;
		if (!ConvertSheet(Workbook, *Mapping, StringTable != nullptr, GeneratedCsvPath, CleanCsvPath, OperationError))
		{
			LogEntry.Message = OperationError;
			bAllSucceeded = false;
			continue;
		}

		bool bImported = false;
		if (DataTable)
		{
			bImported = ImportDataTable(DataTable, CleanCsvPath, OperationError);
			if (bImported)
			{
				if (DataTable->AssetImportData)
				{
					DataTable->AssetImportData->Update(GeneratedCsvPath);
				}
				DataTable->MarkPackageDirty();
				bImported = SaveImportedAsset(DataTable, OperationError);
			}
		}
		else
		{
			bImported = ULastFPSStringTableSyncLibrary::ImportStringTableFromCsv(StringTable, CleanCsvPath);
			if (!bImported)
			{
				OperationError = FText::Format(
					LOCTEXT("StringTableImportFailed", "StringTable 갱신에 실패했습니다: {0}"),
					FText::FromString(Mapping->TargetAsset.ToString()));
			}
			else
			{
				bImported = SaveImportedAsset(StringTable, OperationError);
			}
		}

		LogEntry.bSucceeded = bImported;
		LogEntry.Message = bImported
			? FText::Format(
				LOCTEXT("ImportSucceeded", "{0}을(를) 갱신하고 저장했습니다. CSV={1}"),
				FText::FromName(Sheet.SheetName), FText::FromString(GeneratedCsvPath))
			: OperationError;
		bAllSucceeded &= bImported;
	}
	return bAllSucceeded;
}

bool FLastFPSDataTableImportService::AddRegistryMapping(
	const FName WorkbookName,
	const FName SheetName,
	const FString& CsvFileName,
	const FSoftObjectPath& TargetAssetPath,
	FText& OutError)
{
	ULastFPSDataTableImportRegistry* Registry = LoadRegistry(OutError);
	if (!Registry)
	{
		return false;
	}
	if (!Registry->AddMapping(WorkbookName, SheetName, CsvFileName, TargetAssetPath))
	{
		OutError = LOCTEXT("AddMappingFailed", "레지스트리 항목을 추가하지 못했습니다. 중복 항목, CSV 파일명과 대상 에셋을 확인하세요.");
		return false;
	}

	UEditorAssetSubsystem* AssetSubsystem = GEditor
		? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()
		: nullptr;
	if (!AssetSubsystem || !AssetSubsystem->SaveLoadedAsset(Registry, true))
	{
		OutError = LOCTEXT("SaveRegistryFailed", "레지스트리 항목은 추가했지만 에셋 저장에 실패했습니다.");
		return false;
	}
	OutError = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
