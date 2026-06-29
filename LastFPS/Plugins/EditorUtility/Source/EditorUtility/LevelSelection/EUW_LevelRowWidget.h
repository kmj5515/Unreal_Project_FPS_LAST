#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "EditorUtilityWidget.h"
#include "EUW_LevelHelper.h"
#include "EUW_LevelRowWidget.generated.h"

/** 레벨 선택 목록의 개별 항목 위젯입니다. */
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

    // 전체 경로 표시용 텍스트입니다.
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
