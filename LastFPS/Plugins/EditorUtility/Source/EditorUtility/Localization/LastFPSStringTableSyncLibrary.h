#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LastFPSStringTableSyncLibrary.generated.h"

class UStringTable;

/**
 * String Table 에셋을 CSV 원본으로부터 갱신한다.
 *
 * 엔진은 이 경로를 String Table 에디터의 Import 버튼으로만 노출하고 스크립트에는 열어두지 않아,
 * 로컬라이즈 문구를 고칠 때마다 사람이 에디터를 열어 클릭해야 했다. 자동화가 가능하도록 감싼다.
 *
 * 대상 에셋과 CSV 경로를 인자로 받는다 — 특정 에셋을 아는 순간 다른 String Table 에 재사용할 수 없게 된다.
 */
UCLASS()
class EDITORUTILITY_API ULastFPSStringTableSyncLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * CSV 를 읽어 String Table 항목 전체를 교체하고 에셋을 더티로 표시한다. 저장은 호출부의 몫이다.
	 *
	 * 엔진 구현이 기존 항목을 모두 지우고 다시 채우므로 CSV 는 부분 갱신본이 아니라 완전한 원본이어야 한다.
	 * CSV 에 없는 키는 조용히 사라지며, 그 손실을 여기서 되돌릴 방법은 없다.
	 *
	 * @param StringTable  갱신할 String Table 에셋
	 * @param CsvFilePath  'Key' 와 'SourceString' 열을 가진 CSV 의 절대 경로
	 * @return 갱신에 성공하면 true
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Localization")
	static bool ImportStringTableFromCsv(UStringTable* StringTable, const FString& CsvFilePath);
};
