#include "Data/Tables/LastFPSLoadingTipData.h"

namespace
{
	/** 연속 중복 노출 방지용 — 직전 선택 인덱스 (프로세스 전역, 세션 한정). */
	int32 GLastLoadingTipIndex = INDEX_NONE;
}

bool ULastFPSLoadingTipLibrary::GetRandomLoadingTip(const UDataTable* TipTable, FLastFPSLoadingTipData& OutTip)
{
	if (!TipTable)
	{
		return false;
	}

	TArray<FLastFPSLoadingTipData*> Rows;
	TipTable->GetAllRows<FLastFPSLoadingTipData>(TEXT("GetRandomLoadingTip"), Rows);
	if (Rows.Num() == 0)
	{
		return false;
	}

	int32 Index = 0;
	if (Rows.Num() == 1)
	{
		Index = 0;
	}
	else
	{
		// 직전 인덱스를 제외하고 선택.
		do
		{
			Index = FMath::RandRange(0, Rows.Num() - 1);
		}
		while (Index == GLastLoadingTipIndex);
	}

	GLastLoadingTipIndex = Index;

	if (!Rows[Index])
	{
		return false;
	}

	OutTip = *Rows[Index];
	return true;
}

FText ULastFPSLoadingTipLibrary::GetRandomLoadingTipText(const UDataTable* TipTable)
{
	FLastFPSLoadingTipData Tip;
	return GetRandomLoadingTip(TipTable, Tip) ? Tip.Tip : FText::GetEmpty();
}
