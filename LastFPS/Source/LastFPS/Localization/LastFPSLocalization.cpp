#include "Localization/LastFPSLocalization.h"

#include "UObject/Class.h"

namespace
{
	// String Table 에셋의 등록 ID는 에셋 이름이 아니라 전체 오브젝트 경로다.
	const FName UIStringTableId(TEXT("/Game/Data/Tables/Localize/ST_Localize.ST_Localize"));
}

FText FLastFPSLocalization::GetUIText(const TCHAR* Key)
{
	return FText::FromStringTable(UIStringTableId, FTextKey(Key));
}

FText FLastFPSLocalization::GetUIText(const FName Key)
{
	return FText::FromStringTable(UIStringTableId, FTextKey(Key.ToString()));
}

FText FLastFPSLocalization::GetUIEnumText(const UEnum* EnumType, int64 Value)
{
	if (!EnumType || !EnumType->IsValidEnumValue(Value))
	{
		return FText::GetEmpty();
	}

	const FString Key = FString::Printf(
		TEXT("Enum.%s.%s"),
		*EnumType->GetName(),
		*EnumType->GetNameStringByValue(Value));
	return GetUIText(*Key);
}
