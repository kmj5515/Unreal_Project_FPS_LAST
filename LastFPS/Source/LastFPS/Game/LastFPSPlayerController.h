#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "UI/LastFPSConfirmWidget.h"
#include "LastFPSPlayerController.generated.h"

class APawn;
class ULastFPSCharacterSelectWidget;
class ULastFPSHUDWidget;
class ULastFPSMainMenuWidget;
class ULastFPSNoticeWidget;

DECLARE_DYNAMIC_DELEGATE_OneParam(FLastFPSConfirmResultDelegate, bool, bConfirmed);

UCLASS()
class LASTFPS_API ALastFPSPlayerController : public ACommonPlayerController
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void ShowHitMarker();

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    ULastFPSHUDWidget* GetHUDWidget() const { return HUDWidget; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    ULastFPSMainMenuWidget* GetMainMenuWidget() const { return MainMenuWidget; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI", meta=(AutoCreateRefTerm="OnResult"))
    void ShowConfirm(const FText& Title, const FText& Message, FLastFPSConfirmResultDelegate OnResult);

    UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
    void ShowNotice(const FText& Title, const FText& Message);

    UFUNCTION(BlueprintCallable, Category="LastFPS|Character")
    void SetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    int32 GetSelectedCharacterIndex() const { return SelectedCharacterIndex; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    TSubclassOf<APawn> GetSelectedCharacterClass() const;

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    const TArray<TSubclassOf<APawn>>& GetSelectableCharacterClasses() const { return SelectableCharacterClasses; }

    UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Character")
    void OnSelectedCharacterIndexChanged(int32 NewSelectedCharacterIndex);

protected:
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSHUDWidget> HUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSCharacterSelectWidget> CharacterSelectWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSConfirmWidget> ConfirmWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    TSubclassOf<ULastFPSNoticeWidget> NoticeWidgetClass;

    /** 인게임 HUD (Hub 등) */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bPushHUDOnBeginPlay = false;

    /** 메인 메뉴 UI */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bPushMainMenuOnBeginPlay = false;

    /** 캐릭터 선택 UI */
    UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
    bool bPushCharacterSelectOnBeginPlay = false;

    void TryPushHUDToUILayout();
    void TryPushMainMenuToUILayout();
    void TryPushCharacterSelectToUILayout();

    UFUNCTION()
    void RetryPushHUDToUILayout();

    UFUNCTION()
    void RetryPushMainMenuToUILayout();

    UFUNCTION()
    void RetryPushCharacterSelectToUILayout();

    template<typename TWidget>
    TWidget* PushWidgetToModalLayer(TSubclassOf<TWidget> WidgetClass);

    int32 ClampSelectedCharacterIndex(int32 NewIndex) const;
    void SyncSelectedCharacterState(int32 CharacterIndex);

    UFUNCTION(Server, Reliable)
    void ServerSetSelectedCharacterIndex(int32 NewIndex);

    UFUNCTION()
    void OnRep_SelectedCharacterIndex();

    UPROPERTY()
    TObjectPtr<ULastFPSHUDWidget> HUDWidget;

    UPROPERTY()
    TObjectPtr<ULastFPSMainMenuWidget> MainMenuWidget;

    UPROPERTY()
    TObjectPtr<ULastFPSCharacterSelectWidget> CharacterSelectWidget;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Character")
    TArray<TSubclassOf<APawn>> SelectableCharacterClasses;

    UPROPERTY(ReplicatedUsing=OnRep_SelectedCharacterIndex, BlueprintReadOnly, Category="LastFPS|Character")
    int32 SelectedCharacterIndex = 0;

    FTimerHandle HUDPushRetryTimerHandle;
    FTimerHandle MainMenuPushRetryTimerHandle;
    FTimerHandle CharacterSelectPushRetryTimerHandle;
    bool bHUDWidgetPushed = false;
    bool bMainMenuWidgetPushed = false;
    bool bCharacterSelectWidgetPushed = false;
};
