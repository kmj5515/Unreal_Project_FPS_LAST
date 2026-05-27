#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "EditorUtilityWidget.h"
#include "EUW_LevelHelper.h"
#include "EUW_LevelRowWidget.generated.h"

/**
 * 에디터 레벨 선택 목록의 개별 항목 위젯
 */
UCLASS(Blueprintable, BlueprintType, meta=(ShowWorldContextPin, IsBlueprintBase="true"))
class EDITORUTILITY_API UEUW_LevelRowWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    void SetMapInfo(const FEUW_MapAssetInfo& InInfo);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UCheckBox> FavoriteCheckBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UTextBlock> MapNameText;

    // 풀 패스 표시를 위한 텍스트 추가
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UTextBlock> FullPathText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UButton> OpenLevelButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget, OptionalWidget = true))
    TObjectPtr<class UButton> SetAsStartLevelButton;

private:
    UFUNCTION()
    void HandleFavoriteChanged(bool bIsChecked);

    UFUNCTION()
    void HandleOpenLevelClicked();

    UFUNCTION()
    void HandleSetAsStartLevelClicked();

    FEUW_MapAssetInfo MapInfo;
};
#endif
