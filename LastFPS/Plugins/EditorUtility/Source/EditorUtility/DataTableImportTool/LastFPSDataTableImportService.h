#pragma once

#include "CoreMinimal.h"

// 임포트 로그는 서비스와 모듈 진입점이 함께 사용하므로 파일별 static 정의 대신 모듈 공용 카테고리로 선언한다.
EDITORUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogLastFPSDataTableImport, Log, All);

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
	/** 원본 에셋을 변경하지 않고 현재 DT 또는 Excel 반영 예정값의 transient 복사본을 만든다. */
	static class UDataTable* CreateDataTablePreview(
		const FLastFPSDataTableImportWorkbookInfo& Workbook,
		FName SheetName,
		bool bUseExcelValues,
		FText& OutError);
	static FString GetExcelDirectory();
	static FString GetCsvDirectory();

private:
	static class ULastFPSDataTableImportRegistry* LoadRegistry(FText& OutError);
};
