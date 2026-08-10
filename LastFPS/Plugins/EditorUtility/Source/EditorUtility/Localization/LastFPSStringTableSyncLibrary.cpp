#include "Localization/LastFPSStringTableSyncLibrary.h"

#include "HAL/FileManager.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStringTableSync, Log, All);

bool ULastFPSStringTableSyncLibrary::ImportStringTableFromCsv(UStringTable* StringTable, const FString& CsvFilePath)
{
	if (!StringTable)
	{
		UE_LOG(LogLastFPSStringTableSync, Error,
			TEXT("String Table 갱신에 실패했습니다: 대상 에셋이 null 입니다. CSV=%s"),
			*CsvFilePath);
		return false;
	}

	// 파일 부재는 엔진 쪽에서도 경고만 남기고 false 를 돌려주지만, 어떤 에셋을 갱신하려다 실패했는지는
	// 그 로그에 없다. 호출부가 원인을 알 수 있도록 여기서 먼저 걸러낸다.
	if (!IFileManager::Get().FileExists(*CsvFilePath))
	{
		UE_LOG(LogLastFPSStringTableSync, Error,
			TEXT("String Table '%s' 갱신에 실패했습니다: CSV 를 찾을 수 없습니다. 경로=%s"),
			*StringTable->GetPathName(), *CsvFilePath);
		return false;
	}

	if (!StringTable->GetMutableStringTable()->ImportStrings(CsvFilePath))
	{
		UE_LOG(LogLastFPSStringTableSync, Error,
			TEXT("String Table '%s' 갱신에 실패했습니다: CSV 해석에 실패했습니다('Key'/'SourceString' 열 확인). 경로=%s"),
			*StringTable->GetPathName(), *CsvFilePath);
		return false;
	}

	StringTable->MarkPackageDirty();

	UE_LOG(LogLastFPSStringTableSync, Log,
		TEXT("String Table '%s' 을(를) CSV 로 갱신했습니다: %s"),
		*StringTable->GetPathName(), *CsvFilePath);
	return true;
}
