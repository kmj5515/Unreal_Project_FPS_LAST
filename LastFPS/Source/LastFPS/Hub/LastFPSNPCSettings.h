#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSNPCSettings.generated.h"

class UDataTable;
class UUserWidget;

/**
 * NPC 데이터 테이블을 프로젝트 전역에서 한 번만 지정하는 설정.
 * (Project Settings > MWTool > NPC)
 * 레벨에 배치된 ALastFPSNPC 가 BeginPlay 에 NPCRowName 으로 여기 테이블을 조회해 스스로 설정한다.
 */
UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS NPC"))
class LASTFPS_API ULastFPSNPCSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** NPC 정의 테이블 (FLastFPSNPCSpawnData) */
	UPROPERTY(Config, EditAnywhere, Category="NPC", meta=(AllowedClasses="/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> NPCDataTable;

	/** 대화 테이블 (FLastFPSDialogueData) — 액션의 DialogueRowName 해석용 */
	UPROPERTY(Config, EditAnywhere, Category="NPC", meta=(AllowedClasses="/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> DialogueTable;

	/** 머리 위 마커 위젯 (WBP_NPCMarker) — BP에 마커가 지정 안 됐을 때 공통으로 적용 */
	UPROPERTY(Config, EditAnywhere, Category="NPC", meta=(AllowedClasses="/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> MarkerWidgetClass;

#if WITH_EDITOR
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif
};
