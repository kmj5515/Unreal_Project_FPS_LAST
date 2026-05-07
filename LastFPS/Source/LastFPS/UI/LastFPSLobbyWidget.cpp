#include "UI/LastFPSLobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/LastFPSPlayerController.h"
#include "Game/LastFPSLobbyGameMode.h"
#include "Game/LastFPSLobbyGameState.h"

void ULastFPSLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_Ready)
    {
        Button_Ready->OnClicked.AddDynamic(this, &ULastFPSLobbyWidget::HandleReadyClicked);
    }
    if (Button_C1)
    {
        Button_C1->OnClicked.AddDynamic(this, &ULastFPSLobbyWidget::HandleCharacter1Clicked);
    }
    if (Button_C2)
    {
        Button_C2->OnClicked.AddDynamic(this, &ULastFPSLobbyWidget::HandleCharacter2Clicked);
    }
    if (Button_C3)
    {
        Button_C3->OnClicked.AddDynamic(this, &ULastFPSLobbyWidget::HandleCharacter3Clicked);
    }

    UpdateStatusText(TEXT("로비"));

    CachedSelectedCharacterIndex = GetSelectedCharacterIndex();
    if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
    {
        OnSelectedCharacterChanged(PC->GetSelectedCharacterClass(), CachedSelectedCharacterIndex);
    }
}

void ULastFPSLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!Text_TimeRemaining && !Text_Status)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (ALastFPSLobbyGameState* LobbyGS = World->GetGameState<ALastFPSLobbyGameState>())
    {
        if (Text_Status)
        {
            UpdateStatusText(BuildLobbyStatusText(
                LobbyGS->PlayerArray.Num(),
                LobbyGS->LobbyStartPlayerCount,
                LobbyGS->bTravelTriggered,
                LobbyGS->bTeamIntroInProgress,
                LobbyGS->bCharacterSelectInProgress));
        }

        UpdateRemainingTimeText(LobbyGS->bCharacterSelectInProgress, LobbyGS->RemainingCharacterSelectSeconds);
    }
    else if (ALastFPSLobbyGameMode* LobbyGM = Cast<ALastFPSLobbyGameMode>(World->GetAuthGameMode()))
    {
        if (Text_Status)
        {
            UpdateStatusText(BuildLobbyStatusText(
                LobbyGM->GetTotalConnectedPlayers(),
                LobbyGM->GetLobbyStartPlayerCount(),
                LobbyGM->IsTravelTriggered(),
                LobbyGM->IsTeamIntroInProgress(),
                LobbyGM->IsCharacterSelectInProgress()));
        }

        UpdateRemainingTimeText(LobbyGM->IsCharacterSelectInProgress(), LobbyGM->GetRemainingCharacterSelectSeconds());
    }

    const int32 CurrentSelectedIndex = GetSelectedCharacterIndex();
    if (CurrentSelectedIndex != CachedSelectedCharacterIndex)
    {
        CachedSelectedCharacterIndex = CurrentSelectedIndex;
        if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
        {
            OnSelectedCharacterChanged(PC->GetSelectedCharacterClass(), CurrentSelectedIndex);
        }
    }
}

FString ULastFPSLobbyWidget::BuildPhaseText(bool bTravelTriggered, bool bTeamIntroInProgress, bool bCharacterSelectInProgress) const
{
    if (bTravelTriggered)
    {
        return TEXT("이동 중");
    }
    if (bTeamIntroInProgress)
    {
        return TEXT("팀 인트로");
    }
    if (bCharacterSelectInProgress)
    {
        return TEXT("캐릭터 선택");
    }

    return TEXT("대기");
}

FString ULastFPSLobbyWidget::BuildLobbyStatusText(
    int32 CurrentPlayers,
    int32 NeededPlayers,
    bool bTravelTriggered,
    bool bTeamIntroInProgress,
    bool bCharacterSelectInProgress) const
{
    const FString PhaseText = BuildPhaseText(bTravelTriggered, bTeamIntroInProgress, bCharacterSelectInProgress);
    const FString ReadyText = bIsReady ? TEXT("Ready") : TEXT("Not Ready");
    return FString::Printf(TEXT("%s | 인원 %d/%d | %s"), *PhaseText, CurrentPlayers, NeededPlayers, *ReadyText);
}

void ULastFPSLobbyWidget::UpdateRemainingTimeText(bool bCharacterSelectInProgress, int32 RemainingSeconds)
{
    if (!Text_TimeRemaining)
    {
        return;
    }

    if (bCharacterSelectInProgress)
    {
        Text_TimeRemaining->SetText(FText::FromString(FString::Printf(TEXT("남은 시간: %d"), RemainingSeconds)));
        return;
    }

    Text_TimeRemaining->SetText(FText::GetEmpty());
}

void ULastFPSLobbyWidget::HandleReadyClicked()
{
    if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
    {
        bIsReady = !bIsReady;
        PC->SetLobbyReady(bIsReady);

        if (Button_Ready)
        {
            const FText ButtonText = bIsReady
                ? FText::FromString(TEXT("준비 취소"))
                : FText::FromString(TEXT("준비 하기"));
            if (UTextBlock* ButtonLabel = Cast<UTextBlock>(Button_Ready->GetChildAt(0)))
            {
                ButtonLabel->SetText(ButtonText);
            }
        }

        UpdateStatusText(bIsReady ? TEXT("준비 완료") : TEXT("준비 대기"));
    }
}

void ULastFPSLobbyWidget::UpdateStatusText(const FString& InText)
{
    if (Text_Status)
    {
        Text_Status->SetText(FText::FromString(InText));
    }
}

void ULastFPSLobbyWidget::SelectCharacterByIndex(int32 CharacterIndex)
{
    if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
    {
        PC->SetSelectedCharacterIndex(CharacterIndex);
    }
}

int32 ULastFPSLobbyWidget::GetSelectedCharacterIndex() const
{
    if (const ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
    {
        return PC->GetSelectedCharacterIndex();
    }

    return INDEX_NONE;
}

TArray<TSubclassOf<APawn>> ULastFPSLobbyWidget::GetSelectableCharacterClasses() const
{
    if (const ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
    {
        return PC->GetSelectableCharacterClasses();
    }

    return {};
}

void ULastFPSLobbyWidget::HandleCharacter1Clicked()
{
    SelectCharacterByIndex(0);
}

void ULastFPSLobbyWidget::HandleCharacter2Clicked()
{
    SelectCharacterByIndex(1);
}

void ULastFPSLobbyWidget::HandleCharacter3Clicked()
{
    SelectCharacterByIndex(2);
}

