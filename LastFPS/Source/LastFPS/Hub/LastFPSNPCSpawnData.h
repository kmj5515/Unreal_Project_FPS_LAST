#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Hub/LastFPSNPCTypes.h"
#include "LastFPSNPCSpawnData.generated.h"

class UDataTable;

/**
 * JSON 친화 액션 정의 (DT_NPCData 행에 들어감).
 * 런타임 FLastFPSNPCAction과 달리 오브젝트 참조가 없어 JSON 재임포트에 안전하다.
 * - Dialogue: DialogueRowName 을 DialogueTable 에서 해석
 * - Screen: ScreenTag (UI.Screen.Shop 등)
 */
USTRUCT(BlueprintType)
struct FLastFPSNPCActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	ELastFPSNPCActionType Type = ELastFPSNPCActionType::Dialogue;

	/** Type==Screen 일 때 열 화면 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC", meta=(Categories="UI.Screen"))
	FGameplayTag ScreenTag;

	/** Type==Dialogue 일 때 DT_DialogueData 의 행 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FName DialogueRowName;
};

/**
 * NPC 한 명의 데이터 (DT_NPCData 행). JSON에서 임포트.
 * 레벨에 배치된 ALastFPSNPC 가 자기 NPCRowName 으로 이 행을 조회해 스스로 설정한다.
 * 외형(메시/애님/카메라)은 배치한 BP가 담당 — 데이터엔 에셋 참조가 없다.
 */
USTRUCT(BlueprintType)
struct FLastFPSNPCSpawnData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FText NPCRole;

	/** 머리 위 마커의 G 힌트 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	FText InteractionLabel = NSLOCTEXT("LastFPS", "NPC_DefaultInteract", "대화");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC", meta=(ClampMin="50"))
	float InteractionRadius = 150.f;

	/** 허브 버튼 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	TArray<FLastFPSNPCActionData> Actions;

	/** JSON 액션 목록 → 런타임 액션 목록 (대화 행을 DialogueTable 로 해석) */
	TArray<FLastFPSNPCAction> BuildRuntimeActions(UDataTable* DialogueTable) const;
};
