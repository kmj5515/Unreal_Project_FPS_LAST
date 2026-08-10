#pragma once

#include "CoreMinimal.h"

enum class ELastFPSDataTableImportSheetState : uint8
{
	Ready,
	Excluded,
	Unregistered,
	InvalidMapping
};

// 스캔 결과는 UObject 수명과 분리된 값으로 전달해 서비스가 Slate 위젯의 상태를 소유하지 않게 한다.
struct FLastFPSDataTableImportSheetInfo
{
	FName SheetName;
	ELastFPSDataTableImportSheetState State = ELastFPSDataTableImportSheetState::Unregistered;
	FText StateMessage;
	FSoftObjectPath TargetAssetPath;
	FString CsvFileName;
};

struct FLastFPSDataTableImportWorkbookInfo
{
	FName WorkbookName;
	FString WorkbookPath;
	FDateTime ModifiedTime;
	bool bModifiedSinceImport = false;
	bool bHasUnregisteredSheets = false;
	TArray<FLastFPSDataTableImportSheetInfo> Sheets;
};

struct FLastFPSDataTableImportLogEntry
{
	FName SheetName;
	bool bSucceeded = false;
	FText Message;
};

class EDITORUTILITY_API FLastFPSDataTableImportService
{
public:
	// 파일 탐색과 에셋 변경을 한 경계에 모아 UI가 임포트 구현 세부 사항에 의존하지 않게 한다.
	static bool ScanWorkbooks(TArray<FLastFPSDataTableImportWorkbookInfo>& OutWorkbooks, FText& OutError);
	static bool ImportSheets(
		const FLastFPSDataTableImportWorkbookInfo& Workbook,
		const TSet<FName>& SelectedSheets,
		TArray<FLastFPSDataTableImportLogEntry>& OutLogEntries);
	static bool AddRegistryMapping(
		FName WorkbookName,
		FName SheetName,
		const FString& CsvFileName,
		const FSoftObjectPath& TargetAssetPath,
		FText& OutError);
	static FString GetExcelDirectory();

private:
	static class ULastFPSDataTableImportRegistry* LoadRegistry(FText& OutError);
};
