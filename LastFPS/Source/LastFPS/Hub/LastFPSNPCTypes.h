#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "LastFPSNPCTypes.generated.h"

/** NPC 허브 버튼 하나가 하는 일의 종류 */
UENUM(BlueprintType)
enum class ELastFPSNPCActionType : uint8
{
	/** 대사를 무전 자막으로 재생 (DialogueRow) */
	Dialogue	UMETA(DisplayName="대화"),
	/** 태그로 화면 열기 (ScreenTag — 상점/모듈 등) */
	Screen		UMETA(DisplayName="화면 열기")
};

/**
 * 상호작용 화면에서 이 버튼이 놓일 탭.
 * 임무 탭은 진행 상황에 따라 런타임으로 채워지므로, 데이터로 지정하는 버튼은 보통 대화 탭에 둔다.
 */
UENUM(BlueprintType)
enum class ELastFPSNPCActionTab : uint8
{
	Talk	UMETA(DisplayName="대화"),
	Quest	UMETA(DisplayName="임무")
};

/**
 * NPC 상호작용 허브의 버튼 1개 정의.
 * NPC마다 이 배열을 구성해 "대화하기 / 상점 / 모듈" 같은 메뉴를 만든다.
 */
USTRUCT(BlueprintType)
struct FLastFPSNPCAction
{
	GENERATED_BODY()

	/** 버튼 라벨 (예: "대화하기", "상점", "모듈") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	ELastFPSNPCActionType Type = ELastFPSNPCActionType::Dialogue;

	/** 이 버튼이 놓일 탭. 임무 탭은 진행 상황으로 자동 구성되므로 기본은 대화 탭이다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	ELastFPSNPCActionTab Tab = ELastFPSNPCActionTab::Talk;

	/** Type==Screen 일 때 열 화면 태그 (UI.Screen.Shop, UI.Screen.Module ...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC",
		meta=(Categories="UI.Screen", EditCondition="Type==ELastFPSNPCActionType::Screen", EditConditionHides))
	FGameplayTag ScreenTag;

	/** Type==Dialogue 일 때 표시할 대화 행 (FLastFPSDialogueData) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC",
		meta=(RowType="/Script/LastFPS.LastFPSDialogueData", EditCondition="Type==ELastFPSNPCActionType::Dialogue", EditConditionHides))
	FDataTableRowHandle DialogueRow;
};
