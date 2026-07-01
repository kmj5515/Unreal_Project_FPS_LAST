// NPC preset blueprint generator — creates BP_<RowName> children of ALastFPSNPC from a DataTable.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NPCPresetGenLibrary.generated.h"

class UDataTable;

/** Result summary for NPC preset generation, returned to Blueprint / Editor Utility Widgets. */
USTRUCT(BlueprintType)
struct FNPCPresetGenSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPCPresetGen")
	int32 NumCreated = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NPCPresetGen")
	int32 NumSkipped = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NPCPresetGen")
	FString Message;
};

/**
 * NPC 프리셋 BP 생성기.
 * DT_NPCData 의 각 행마다 BP_<RowName> (예: NPC_Mechanic → BP_NPC_Mechanic) 를
 * ALastFPSNPC 자식으로 생성하고 NPCRowName 기본값을 세팅한다.
 * (메시/애님/카메라는 생성 후 각 BP에서 지정 — 외형은 눈으로)
 * Editor Utility Widget(Blutility)에서 호출하도록 설계.
 */
UCLASS()
class WIDGETTREEGEN_API UNPCPresetGenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @param NPCTable          DT_NPCData (행 이름을 읽음 — 행 구조체 종류와 무관)
	 * @param SavePath          생성 위치 (예: /Game/NPC). 비우면 /Game/NPC
	 * @param AssetPrefix       접두사 (기본 "BP_" → BP_NPC_Mechanic)
	 * @param bOverwriteExisting 이미 있으면 삭제 후 재생성 (false면 건너뜀)
	 */
	UFUNCTION(BlueprintCallable, Category = "NPCPresetGen",
		meta = (DisplayName = "Generate NPC Preset Blueprints From Table"))
	static FNPCPresetGenSummary GenerateFromTable(
		UDataTable* NPCTable,
		const FString& SavePath,
		const FString& AssetPrefix,
		bool bOverwriteExisting = false);
};
