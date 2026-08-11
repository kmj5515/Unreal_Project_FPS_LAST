#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSNPCSettings.generated.h"

class UUserWidget;

/**
 * NPC 머리 위 마커 위젯을 프로젝트 전역에서 지정하는 설정.
 * (Project Settings > MWTool > NPC)
 */
UCLASS(Config=LastFPS, DefaultConfig, meta=(DisplayName="LastFPS NPC"))
class LASTFPS_API ULastFPSNPCSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 머리 위 마커 위젯 (WBP_NPCMarker) — BP에 마커가 지정 안 됐을 때 공통으로 적용 */
	UPROPERTY(Config, EditAnywhere, Category="NPC", meta=(AllowedClasses="/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> MarkerWidgetClass;

	/** 퀘스트 수락 가능/목표 상태를 표시하는 NPC 머리 위 위젯 클래스. */
	UPROPERTY(Config, EditAnywhere, Category="Quest Marker", meta=(AllowedClasses="/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> QuestMarkerWidgetClass;

	/** NPC 루트 기준 퀘스트 마커 높이(cm). */
	UPROPERTY(Config, EditAnywhere, Category="Quest Marker", meta=(ClampMin="0"))
	float QuestMarkerHeight = 180.f;

#if WITH_EDITOR
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif
};
