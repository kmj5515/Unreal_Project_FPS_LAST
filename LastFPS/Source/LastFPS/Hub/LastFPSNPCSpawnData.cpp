#include "Hub/LastFPSNPCSpawnData.h"

TArray<FLastFPSNPCAction> FLastFPSNPCSpawnData::BuildRuntimeActions(UDataTable* DialogueTable) const
{
	TArray<FLastFPSNPCAction> Result;
	Result.Reserve(Actions.Num());

	for (const FLastFPSNPCActionData& Data : Actions)
	{
		FLastFPSNPCAction Action;
		Action.Label = Data.Label;
		Action.Type = Data.Type;
		Action.ScreenTag = Data.ScreenTag;

		// 대화 액션은 행 이름을 DialogueTable 과 묶어 런타임 핸들로 만든다.
		if (Data.Type == ELastFPSNPCActionType::Dialogue && !Data.DialogueRowName.IsNone())
		{
			Action.DialogueRow.DataTable = DialogueTable;
			Action.DialogueRow.RowName = Data.DialogueRowName;
		}

		Result.Add(Action);
	}

	return Result;
}
