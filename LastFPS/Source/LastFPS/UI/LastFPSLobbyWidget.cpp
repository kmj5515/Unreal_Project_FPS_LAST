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

    if (ALastFPSLobbyGameMode* LobbyGM = Cast<ALastFPSLobbyGameMode>(World->GetAuthGameMode()))
    {
        if (Text_Status)
        {
            const int32 CurrentPlayers = LobbyGM->GetTotalConnectedPlayers();
            const int32 NeededPlayers = LobbyGM->GetLobbyStartPlayerCount();

            FString PhaseText = TEXT("대기");
            if (LobbyGM->IsTravelTriggered())
            {
                PhaseText = TEXT("이동 중");
            }
            else if (LobbyGM->IsTeamIntroInProgress())
            {
                PhaseText = TEXT("팀 인트로");
            }
            else if (LobbyGM->IsCharacterSelectInProgress())
            {
                PhaseText = TEXT("캐릭터 선택");
            }

            const FString ReadyText = bIsReady ? TEXT("Ready") : TEXT("Not Ready");
            UpdateStatusText(FString::Printf(TEXT("%s | 인원 %d/%d | %s"), *PhaseText, CurrentPlayers, NeededPlayers, *ReadyText));
        }

        if (LobbyGM->IsCharacterSelectInProgress())
        {
            const int32 Remaining = LobbyGM->GetRemainingCharacterSelectSeconds();
            if (Text_TimeRemaining)
            {
                Text_TimeRemaining->SetText(
                    FText::FromString(FString::Printf(TEXT("남은 시간: %d"), Remaining)));
            }
        }
        else
        {
            if (Text_TimeRemaining)
            {
                Text_TimeRemaining->SetText(FText::FromString(TEXT("")));
            }
        }
    }

    if (ALastFPSLobbyGameState* LobbyGS = World->GetGameState<ALastFPSLobbyGameState>())
    {
        if (Text_Status)
        {
            const int32 CurrentPlayers = LobbyGS->PlayerArray.Num();
            const int32 NeededPlayers = LobbyGS->LobbyStartPlayerCount;

            FString PhaseText = TEXT("대기");
            if (LobbyGS->bTravelTriggered)
            {
                PhaseText = TEXT("이동 중");
            }
            else if (LobbyGS->bTeamIntroInProgress)
            {
                PhaseText = TEXT("팀 인트로");
            }
            else if (LobbyGS->bCharacterSelectInProgress)
            {
                PhaseText = TEXT("캐릭터 선택");
            }

            const FString ReadyText = bIsReady ? TEXT("Ready") : TEXT("Not Ready");
            UpdateStatusText(FString::Printf(TEXT("%s | 인원 %d/%d | %s"), *PhaseText, CurrentPlayers, NeededPlayers, *ReadyText));
        }

        if (Text_TimeRemaining)
        {
            if (LobbyGS->bCharacterSelectInProgress)
            {
                Text_TimeRemaining->SetText(
                    FText::FromString(FString::Printf(TEXT("남은 시간: %d"), LobbyGS->RemainingCharacterSelectSeconds)));
            }
            else
            {
                Text_TimeRemaining->SetText(FText::FromString(TEXT("")));
            }
        }
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

