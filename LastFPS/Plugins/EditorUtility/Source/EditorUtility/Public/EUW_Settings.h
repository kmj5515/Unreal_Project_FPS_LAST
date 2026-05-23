#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EUW_Settings.generated.h"

class UEditorUtilityWidgetBlueprint;

/**
 * LastFPS 에디터 툴 설정을 위한 클래스.
 * 프로젝트 세팅 -> LastFPS Editor Tools에서 위젯을 할당할 수 있습니다.
 */
UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="LastFPS Editor Tools"))
class EDITORUTILITY_API UEUW_Settings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UEUW_Settings();

    // 에디터에서 사용할 레벨 선택 툴 위젯 블루프린트
    UPROPERTY(Config, EditAnywhere, Category="Tools", meta=(AllowedClasses="/Script/Blutility.EditorUtilityWidgetBlueprint"))
    TSoftObjectPtr<UEditorUtilityWidgetBlueprint> LevelSelectionTool;

    /** 싱글톤 인스턴스 반환 */
    static const UEUW_Settings* Get() { return GetDefault<UEUW_Settings>(); }
};
