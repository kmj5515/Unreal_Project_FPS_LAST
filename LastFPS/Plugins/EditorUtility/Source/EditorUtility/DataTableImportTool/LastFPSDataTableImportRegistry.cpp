#include "DataTableImportTool/LastFPSDataTableImportRegistry.h"

#include "Misc/Paths.h"

bool FLastFPSDataTableImportMapping::IsValidCsvFileName(const FString& Candidate)
{
	// 시트명 기반 자동 유도는 이름이 다른 기존 원본을 고아로 만들 수 있으므로 명시된 단일 CSV 파일명만 계약으로 인정한다.
	return !Candidate.IsEmpty()
		&& Candidate.Equals(FPaths::GetCleanFilename(Candidate), ESearchCase::CaseSensitive)
		&& FPaths::GetExtension(Candidate).Equals(TEXT("csv"), ESearchCase::IgnoreCase);
}

const FLastFPSDataTableImportMapping* ULastFPSDataTableImportRegistry::FindMapping(
	const FName WorkbookName,
	const FName SheetName) const
{
	return Mappings.FindByPredicate([WorkbookName, SheetName](const FLastFPSDataTableImportMapping& Mapping)
	{
		return Mapping.WorkbookName == WorkbookName && Mapping.SheetName == SheetName;
	});
}

bool ULastFPSDataTableImportRegistry::AddMapping(
	const FName WorkbookName,
	const FName SheetName,
	const FString& CsvFileName,
	const FSoftObjectPath& TargetAssetPath)
{
	if (WorkbookName.IsNone() || SheetName.IsNone() || TargetAssetPath.IsNull()
		|| !FLastFPSDataTableImportMapping::IsValidCsvFileName(CsvFileName)
		|| FindMapping(WorkbookName, SheetName))
	{
		return false;
	}

	// 에디터의 실행 취소 이력과 패키지 저장 상태가 함께 유지되어야 수동 레지스트리 편집과 동작이 일치한다.
	Modify();
	FLastFPSDataTableImportMapping& Mapping = Mappings.AddDefaulted_GetRef();
	Mapping.WorkbookName = WorkbookName;
	Mapping.SheetName = SheetName;
	Mapping.CsvFileName = CsvFileName;
	Mapping.TargetAsset = TSoftObjectPtr<UObject>(TargetAssetPath);
	MarkPackageDirty();
	return true;
}
