#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LastFPSDialogueData.generated.h"

/**
 * 단방향 NPC 대화 1건의 데이터 — DataTable 행.
 * 화자 이름 + 여러 줄(페이지). "다음"으로 한 줄씩 넘기고 마지막에 닫힌다.
 * 분기/선택지 없음(단방향). NPC가 RowHandle로 참조한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSDialogueData : public FTableRowBase
{
	GENERATED_BODY()

	/** 대화창에 표시할 화자 이름 (비우면 NPC의 DisplayName 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	FText SpeakerName;

	/** 페이지 단위 대사. "다음" 버튼으로 한 항목씩 진행. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue", meta=(MultiLine=true))
	TArray<FText> Lines;
};
