#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LastFPSUISettings.generated.h"

class ALastFPSPreviewStageActor;
class ULastFPSUIThemeAsset;

/**
 * 아웃게임 UI 전역 설정 — 어떤 테마 에셋을 쓸지 프로젝트 설정에서 지정한다.
 *
 * 위젯이 특정 테마 에셋 경로를 직접 들고 있으면 톤을 교체할 때 모든 위젯을 고쳐야 한다.
 * 지목은 여기서 한 번만 하고, 위젯은 "활성 테마"라는 계약만 소비한다.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="LastFPS UI"))
class LASTFPS_API ULastFPSUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 소프트 참조 — 아웃게임 화면이 열릴 때만 로드되면 된다. */
	UPROPERTY(config, EditAnywhere, Category="Theme", meta=(AllowedClasses="/Script/LastFPS.LastFPSUIThemeAsset"))
	TSoftObjectPtr<ULastFPSUIThemeAsset> ActiveTheme;

	/**
	 * 3D 프리뷰 무대 클래스. 레벨마다 하나가 미리 스폰돼 화면이 열릴 때 대기 없이 쓰인다.
	 *
	 * 무기·캐릭터 어느 쪽을 세우든 같은 무대를 번갈아 쓴다. 비워 두면 C++ 베이스로 폴백한다.
	 * 어떤 무대를 쓸지는 프로젝트 설정이 정하고, 화면은 "무대가 있다"는 계약만 소비한다.
	 */
	UPROPERTY(config, EditAnywhere, Category="Preview")
	TSoftClassPtr<ALastFPSPreviewStageActor> PreviewStageClass;

	// 프리뷰 자세(ABP·대기 애니메이션)는 여기 두지 않는다. ABP 는 스켈레톤에 묶여 영웅마다 달라야 하므로,
	// 메시를 들고 있는 ULastFPSCharacterVisualData 에 함께 둔다.

	static const ULastFPSUISettings* Get() { return GetDefault<ULastFPSUISettings>(); }

	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};
