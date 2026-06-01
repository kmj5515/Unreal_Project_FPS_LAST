#include "Game/LastFPSPlayerController.h"

#include "UI/LastFPSHUDWidget.h"
#include "UI/LastFPSMainMenuWidget.h"
#include "UI/LastFPSNoticeWidget.h"
#include "UI/LastFPSUITags.h"

#include "Engine/World.h"
#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPlayerController, Log, All);

template<typename TWidget>
TWidget* ALastFPSPlayerController::PushWidgetToModalLayer(TSubclassOf<TWidget> WidgetClass)
{
    if (!WidgetClass)
    {
        return nullptr;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout)
    {
        return nullptr;
    }

    return RootLayout->PushWidgetToLayerStack<TWidget>(
        LastFPSUITags::Layer_Modal(),
        WidgetClass);
}

void ALastFPSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSPlayerController, SelectedCharacterIndex);
}

void ALastFPSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    if (bPushMainMenuOnBeginPlay)
    {
        TryPushMainMenuToUILayout();
    }

    if (bPushHUDOnBeginPlay)
    {
        TryPushHUDToUILayout();
    }

    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
        World->GetTimerManager().ClearTimer(MainMenuPushRetryTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void ALastFPSPlayerController::TryPushHUDToUILayout()
{
    if (bHUDWidgetPushed || !HUDWidgetClass)
    {
        return;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                HUDPushRetryTimerHandle,
                this,
                &ALastFPSPlayerController::RetryPushHUDToUILayout,
                0.1f,
                true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HUDPushRetryTimerHandle);
    }

    HUDWidget = RootLayout->PushWidgetToLayerStack<ULastFPSHUDWidget>(
        LastFPSUITags::Layer_Game(),
        HUDWidgetClass);

    if (HUDWidget)
    {
        bHUDWidgetPushed = true;
        UE_LOG(LogLastFPSPlayerController, Log,
            TEXT("HUD pushed to UI.Layer.Game. Class=%s"),
            *HUDWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogLastFPSPlayerController, Error, TEXT("Failed to push HUD widget to layout"));
    }
}

void ALastFPSPlayerController::RetryPushHUDToUILayout()
{
    TryPushHUDToUILayout();
}

void ALastFPSPlayerController::TryPushMainMenuToUILayout()
{
    if (bMainMenuWidgetPushed || !MainMenuWidgetClass)
    {
        return;
    }

    UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(this);
    if (!RootLayout)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                MainMenuPushRetryTimerHandle,
                this,
                &ALastFPSPlayerController::RetryPushMainMenuToUILayout,
                0.1f,
                true);
        }
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MainMenuPushRetryTimerHandle);
    }

    MainMenuWidget = RootLayout->PushWidgetToLayerStack<ULastFPSMainMenuWidget>(
        LastFPSUITags::Layer_Menu(),
        MainMenuWidgetClass);

    if (MainMenuWidget)
    {
        bMainMenuWidgetPushed = true;
        UE_LOG(LogLastFPSPlayerController, Log,
            TEXT("Main menu pushed to UI.Layer.Menu. Class=%s"),
            *MainMenuWidgetClass->GetName());
    }
    else
    {
        UE_LOG(LogLastFPSPlayerController, Error, TEXT("Failed to push main menu widget to layout"));
    }
}

void ALastFPSPlayerController::RetryPushMainMenuToUILayout()
{
    TryPushMainMenuToUILayout();
}

void ALastFPSPlayerController::ShowConfirm(
    const FText& Title,
    const FText& Message,
    FLastFPSConfirmResultDelegate OnResult)
{
    if (ULastFPSConfirmWidget* ConfirmWidget = PushWidgetToModalLayer<ULastFPSConfirmWidget>(ConfirmWidgetClass))
    {
        ConfirmWidget->SetupConfirm(Title, Message);
        if (OnResult.IsBound())
        {
            ConfirmWidget->OnConfirmResult.Add(OnResult);
        }
    }
}

void ALastFPSPlayerController::ShowNotice(const FText& Title, const FText& Message)
{
    if (ULastFPSNoticeWidget* NoticeWidget = PushWidgetToModalLayer<ULastFPSNoticeWidget>(NoticeWidgetClass))
    {
        NoticeWidget->SetupNotice(Title, Message);
    }
}

void ALastFPSPlayerController::ShowHitMarker()
{
    if (HUDWidget)
    {
        HUDWidget->ShowHitMarker();
    }
}

int32 ALastFPSPlayerController::ClampSelectedCharacterIndex(const int32 NewIndex) const
{
    const int32 MaxIndex = SelectableCharacterClasses.Num() > 0 ? (SelectableCharacterClasses.Num() - 1) : NewIndex;
    return FMath::Clamp(NewIndex, 0, MaxIndex);
}

void ALastFPSPlayerController::SyncSelectedCharacterState(const int32 CharacterIndex)
{
    if (ALastFPSPlayerState* LastPS = GetPlayerState<ALastFPSPlayerState>())
    {
        LastPS->Auth_SetSelectedCharacterIndex(CharacterIndex);

        if (ULastFPSGameInstance* LastGI = GetGameInstance<ULastFPSGameInstance>())
        {
            const FString PlayerKey = LastPS->GetPlayerName();
            LastGI->SaveSelectedCharacterIndex(PlayerKey, CharacterIndex);
        }
    }
}

void ALastFPSPlayerController::SetSelectedCharacterIndex(const int32 NewIndex)
{
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);
    SelectedCharacterIndex = ClampedIndex;
    ServerSetSelectedCharacterIndex(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

TSubclassOf<APawn> ALastFPSPlayerController::GetSelectedCharacterClass() const
{
    if (!SelectableCharacterClasses.IsValidIndex(SelectedCharacterIndex))
    {
        return nullptr;
    }

    return SelectableCharacterClasses[SelectedCharacterIndex];
}

void ALastFPSPlayerController::ServerSetSelectedCharacterIndex_Implementation(const int32 NewIndex)
{
    const int32 ClampedIndex = ClampSelectedCharacterIndex(NewIndex);

    SelectedCharacterIndex = ClampedIndex;
    SyncSelectedCharacterState(ClampedIndex);
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}

void ALastFPSPlayerController::OnRep_SelectedCharacterIndex()
{
    OnSelectedCharacterIndexChanged(SelectedCharacterIndex);
}
