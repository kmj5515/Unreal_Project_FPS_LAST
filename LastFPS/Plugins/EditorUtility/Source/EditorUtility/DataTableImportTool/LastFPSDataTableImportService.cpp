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
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogLastFPSDataTableImport);

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
		const FString CsvDirectory = FLastFPSDataTableImportService::GetCsvDirectory();
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

		// 커밋용 CSV는 저장소에 남는 산출물이고, Unreal 임포터에 넘기는 clean CSV는 Saved 아래의 임시본이다.
		// 워크북과 산출물을 한 폴더에 섞으면 원본을 찾기 어려워지므로 CSV는 별도 폴더에 모은다.
		IFileManager::Get().MakeDirectory(*CsvDirectory, true);
		OutGeneratedCsvPath = FPaths::Combine(CsvDirectory, Mapping.CsvFileName);

		const FString WorkingDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DataTableImport"));
		const FString FilePrefix = Workbook.WorkbookName.ToString() + TEXT("_") + Mapping.SheetName.ToString() + TEXT(".csv");
		const FString CleanDirectory = FPaths::Combine(WorkingDirectory, TEXT("Clean"));
		const FString PendingDirectory = FPaths::Combine(WorkingDirectory, TEXT("Pending"));
		IFileManager::Get().MakeDirectory(*CleanDirectory, true);
		IFileManager::Get().MakeDirectory(*PendingDirectory, true);
		OutCleanCsvPath = FPaths::Combine(CleanDirectory, FilePrefix);

		// 커밋용 CSV를 곧바로 덮어쓰면 사람이 손댄 내용인지 판별할 기회가 사라지므로, 먼저 Pending에 만들어 두고 비교한 뒤 반영한다.
		const FString PendingCsvPath = FPaths::Combine(PendingDirectory, FilePrefix);

		TArray<FString> Arguments = {
			TEXT("--project-dir"), FPaths::ProjectDir(),
			TEXT("convert"),
			TEXT("--workbook"), Workbook.WorkbookPath,
			TEXT("--sheet"), Mapping.SheetName.ToString(),
			TEXT("--output"), PendingCsvPath,
			TEXT("--clean-output"), OutCleanCsvPath
		};
		if (bLocalize)
		{
			Arguments.Add(TEXT("--localize"));
		}
		if (!ExecutePython(Arguments, OutError))
		{
			return false;
		}

		FString PendingText;
		if (!FFileHelper::LoadFileToString(PendingText, *PendingCsvPath))
		{
			OutError = FText::Format(
				LOCTEXT("PendingCsvReadFailed", "변환 결과 CSV를 읽을 수 없습니다: {0}"),
				FText::FromString(PendingCsvPath));
			return false;
		}

		// 시각만으로 판단하면 임포트가 직접 만든 CSV까지 사람 편집으로 오인해 이후 임포트가 전부 거부된다.
		// 내용이 실제로 달라졌을 때만 사람 편집으로 보고 거부한다.
		FString ExistingText;
		if (FFileHelper::LoadFileToString(ExistingText, *OutGeneratedCsvPath))
		{
			if (ExistingText.Equals(PendingText, ESearchCase::CaseSensitive))
			{
				return true;
			}

			const FDateTime CsvModifiedTime = IFileManager::Get().GetTimeStamp(*OutGeneratedCsvPath);
			const FDateTime WorkbookModifiedTime = IFileManager::Get().GetTimeStamp(*Workbook.WorkbookPath);
			if (CsvModifiedTime > WorkbookModifiedTime)
			{
				OutError = FText::Format(
					LOCTEXT("CsvNewerThanWorkbook", "기존 CSV가 워크북보다 최신이고 내용도 달라 덮어쓰기를 거부했습니다. 워크북에 반영한 뒤 다시 시도하세요. CSV={0}, 워크북={1}"),
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

		if (IFileManager::Get().Copy(*OutGeneratedCsvPath, *PendingCsvPath, true, true) != COPY_OK)
		{
			OutError = FText::Format(
				LOCTEXT("GeneratedCsvWriteFailed", "커밋용 CSV를 갱신하지 못했습니다. 파일이 열려 있는지 확인하세요: {0}"),
				FText::FromString(OutGeneratedCsvPath));
			return false;
		}
		return true;
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

	// Unreal CSV 임포터는 구조체·태그컨테이너 컬럼의 빈 값을 거부하고 '()'를 요구한다.
	// 기획자가 셀을 비워두는 것은 자연스러운 입력이므로, RowStruct에서 구조체로 확인된 컬럼에 한해 빈 값을 채운다.
	// 컬럼 타입을 근거로 판단하므로 문자열 컬럼의 빈 값은 그대로 둔다.
	FString FillEmptyStructCells(const FString& CsvText, const UScriptStruct* RowStruct)
	{
		if (!RowStruct || CsvText.IsEmpty())
		{
			return CsvText;
		}

		TSet<FString> StructColumnNames;
		for (TFieldIterator<FProperty> PropertyIt(RowStruct); PropertyIt; ++PropertyIt)
		{
			if (PropertyIt->IsA<FStructProperty>())
			{
				StructColumnNames.Add(PropertyIt->GetName());
				StructColumnNames.Add(PropertyIt->GetAuthoredName());
			}
		}
		if (StructColumnNames.Num() == 0)
		{
			return CsvText;
		}

		FString Result;
		Result.Reserve(CsvText.Len() + 64);

		TArray<FString> HeaderNames;
		TSet<int32> StructColumnIndexes;
		FString Field;
		int32 ColumnIndex = 0;
		int32 RowIndex = 0;
		bool bInQuotes = false;
		bool bFieldOpen = true;

		auto CloseField = [&Field, &Result, &HeaderNames, &StructColumnIndexes, &ColumnIndex, &RowIndex, &bFieldOpen]()
		{
			if (!bFieldOpen)
			{
				return;
			}
			if (RowIndex == 0)
			{
				HeaderNames.Add(Field);
			}
			else if (Field.IsEmpty() && StructColumnIndexes.Contains(ColumnIndex))
			{
				Result += TEXT("()");
			}
			Field.Reset();
			bFieldOpen = false;
		};

		for (int32 Index = 0; Index < CsvText.Len(); ++Index)
		{
			const TCHAR Char = CsvText[Index];
			if (bInQuotes)
			{
				Result.AppendChar(Char);
				if (Char == TEXT('"'))
				{
					const bool bEscapedQuote = (Index + 1 < CsvText.Len()) && CsvText[Index + 1] == TEXT('"');
					if (bEscapedQuote)
					{
						Result.AppendChar(CsvText[++Index]);
						continue;
					}
					bInQuotes = false;
					continue;
				}
				Field.AppendChar(Char);
				continue;
			}

			if (Char == TEXT('"'))
			{
				// 인용된 필드는 내용이 비어 보여도 명시적으로 기록된 값이므로 치환 대상에서 제외한다.
				bInQuotes = true;
				bFieldOpen = true;
				Field.AppendChar(Char);
				Result.AppendChar(Char);
				continue;
			}
			if (Char == TEXT(','))
			{
				CloseField();
				Result.AppendChar(Char);
				++ColumnIndex;
				bFieldOpen = true;
				continue;
			}
			if (Char == TEXT('\r') || Char == TEXT('\n'))
			{
				CloseField();
				Result.AppendChar(Char);
				if (Char == TEXT('\n'))
				{
					if (RowIndex == 0)
					{
						for (int32 HeaderIndex = 0; HeaderIndex < HeaderNames.Num(); ++HeaderIndex)
						{
							if (StructColumnNames.Contains(HeaderNames[HeaderIndex]))
							{
								StructColumnIndexes.Add(HeaderIndex);
							}
						}
					}
					++RowIndex;
					ColumnIndex = 0;
					bFieldOpen = true;
				}
				continue;
			}

			Field.AppendChar(Char);
			Result.AppendChar(Char);
			bFieldOpen = true;
		}
		CloseField();

		return Result;
	}

	bool ImportDataTable(UDataTable* DataTable, const FString& CsvPath, FText& OutError)
	{
		FString RawCsvText;
		if (!FFileHelper::LoadFileToString(RawCsvText, *CsvPath))
		{
			OutError = FText::Format(
				LOCTEXT("CsvReadFailed", "임포트용 CSV를 읽을 수 없습니다: {0}"),
				FText::FromString(CsvPath));
			return false;
		}

		const FString CsvText = FillEmptyStructCells(RawCsvText, DataTable->GetRowStruct());

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

FString FLastFPSDataTableImportService::GetCsvDirectory()
{
	const ULastFPSDataTableImportSettings* Settings = ULastFPSDataTableImportSettings::Get();
	return Settings ? Settings->ResolveCsvDirectory() : FString();
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
