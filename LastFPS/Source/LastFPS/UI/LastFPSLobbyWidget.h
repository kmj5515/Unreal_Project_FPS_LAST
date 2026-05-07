#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSLobbyWidget.generated.h"

class UButton;
class UTextBlock;
class APawn;

UCLASS()
class LASTFPS_API ULastFPSLobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Lobby")
    void SelectCharacterByIndex(int32 CharacterIndex);

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    int32 GetSelectedCharacterIndex() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Lobby")
    TArray<TSubclassOf<APawn>> GetSelectableCharacterClasses() const;

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Lobby")
    void OnSelectedCharacterChanged(TSubclassOf<APawn> SelectedClass, int32 SelectedIndex);

protected:
    // WBP_Lobby에서 아래 이름 그대로 만들면 자동 바인딩됨
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UButton> Button_Ready;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UButton> Button_C1;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UButton> Button_C2;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UButton> Button_C3;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Status;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TimeRemaining;

private:
    FString BuildPhaseText(bool bTravelTriggered, bool bTeamIntroInProgress, bool bCharacterSelectInProgress) const;
    FString BuildLobbyStatusText(int32 CurrentPlayers, int32 NeededPlayers, bool bTravelTriggered, bool bTeamIntroInProgress, bool bCharacterSelectInProgress) const;
    void UpdateRemainingTimeText(bool bCharacterSelectInProgress, int32 RemainingSeconds);

    UFUNCTION()
    void HandleReadyClicked();

    UFUNCTION()
    void HandleCharacter1Clicked();

    UFUNCTION()
    void HandleCharacter2Clicked();

    UFUNCTION()
    void HandleCharacter3Clicked();

    void UpdateStatusText(const FString& InText);

    bool bIsReady = false;
    int32 CachedSelectedCharacterIndex = INDEX_NONE;
};

