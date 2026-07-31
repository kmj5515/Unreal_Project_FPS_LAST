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

#if WITH_EDITOR
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif
};
