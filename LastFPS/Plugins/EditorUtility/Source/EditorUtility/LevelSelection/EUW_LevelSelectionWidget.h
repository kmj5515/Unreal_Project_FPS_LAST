#pragma once

#include "CoreMinimal.h"
#if WITH_EDITOR
#include "EditorUtilityWidget.h"
#include "EUW_LevelSelectionWidget.generated.h"

UENUM(BlueprintType)
enum EMapDisplayMode : uint8
{
    All,            // 전체
    Favorite,       // 즐겨찾기
};

/** 레벨 선택 에디터 유틸리티 위젯의 메인 클래스입니다. */
UCLASS(Blueprintable, BlueprintType, meta=(IsBlueprintBase="true"))
class EDITORUTILITY_API UEUW_LevelSelectionWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EUW|Editor")
    void RefreshMapList();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UScrollBox> MapScrollBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UButton> RefreshButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UButton> BrowseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UEditableTextBox> PathTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UButton> AllButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<class UButton> FavoriteButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EUW|Editor")
    TSubclassOf<class UEUW_LevelRowWidget> RowWidgetClass;

private:
    EMapDisplayMode MapDisplayMode = EMapDisplayMode::All;

    void ChangeDisplayMode();

    UFUNCTION()
    void HandleRefreshClicked();

    UFUNCTION()
    void HandleBrowseClicked();

    UFUNCTION()
    void HandlePathCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void HandleAllButtonClicked();

    UFUNCTION()
    void HandleFavoriteButtonClicked();
};
#endif
