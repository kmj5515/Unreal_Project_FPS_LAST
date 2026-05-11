#include "UI/LastFPSScoreboardWidget.h"
#include "UI/LastFPSScoreRowWidget.h"
#include "Game/LastFPSMatchGameState.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
struct FRowEntry
{
    FString PlayerName;
    int32   Kills           = 0;
    int32   Deaths          = 0;
    int32   Assists         = 0;
    int32   DamageDealt     = 0;
    int32   DamageTaken     = 0;
    int32   HealingReceived = 0;
    bool    bIsLocalPlayer  = false;
};
}

void ULastFPSScoreboardWidget::RefreshScoreboard()
{
    const UWorld* World = GetWorld();
    const ALastFPSMatchGameState* MGS = World ? World->GetGameState<ALastFPSMatchGameState>() : nullptr;

    // 매치 결과 헤더 (매치 종료 전에는 빈 텍스트)
    if (MatchResultText)
    {
        if (MGS && MGS->IsMatchEnded())
        {
            const APlayerState* Winner = MGS->GetWinnerPlayerState();
            const FString WinnerLine = Winner
                ? FString::Printf(TEXT("WINNER: %s"), *Winner->GetPlayerName())
                : TEXT("DRAW");
            const FString Header = FString::Printf(TEXT("%s\nReason: %s"), *WinnerLine, *MGS->GetEndReason());
            MatchResultText->SetText(FText::FromString(Header));
        }
        else
        {
            MatchResultText->SetText(FText::GetEmpty());
        }
    }

    // 행 컨테이너가 없거나 행 위젯 클래스가 미설정이면 더 진행 불가 (BP에서 RowsContainer / RowWidgetClass 설정 필요)
    if (!RowsContainer || !RowWidgetClass)
    {
        return;
    }

    RowsContainer->ClearChildren();

    // 1) 헤더 행 — 데이터 행과 동일 클래스/컬럼 폭으로 정렬
    if (ULastFPSScoreRowWidget* HeaderRow = CreateWidget<ULastFPSScoreRowWidget>(this, RowWidgetClass))
    {
        HeaderRow->SetHeader();
        RowsContainer->AddChild(HeaderRow);
    }

    // 2) 플레이어 행 — GameState 부재 시 빈 목록만 표시 (헤더는 그대로 보임)
    if (!MGS)
    {
        return;
    }

    const APlayerController* LocalPC = GetOwningPlayer();
    TArray<FRowEntry> Rows;
    Rows.Reserve(MGS->PlayerArray.Num());

    for (APlayerState* PS : MGS->PlayerArray)
    {
        const ALastFPSPlayerState* LFPS = Cast<ALastFPSPlayerState>(PS);
        if (!LFPS) continue;

        FRowEntry Row;
        Row.PlayerName      = LFPS->GetPlayerName();
        Row.Kills           = LFPS->GetStatKills();
        Row.Deaths          = LFPS->GetStatDeaths();
        Row.Assists         = LFPS->GetStatAssists();
        Row.DamageDealt     = FMath::RoundToInt(LFPS->GetStatDamageDealt());
        Row.DamageTaken     = FMath::RoundToInt(LFPS->GetStatDamageTaken());
        Row.HealingReceived = FMath::RoundToInt(LFPS->GetStatHealingReceived());
        Row.bIsLocalPlayer  = (LocalPC && LFPS == LocalPC->PlayerState);
        Rows.Add(Row);
    }

    Rows.Sort([](const FRowEntry& A, const FRowEntry& B)
    {
        return A.Kills != B.Kills ? A.Kills > B.Kills : A.Deaths < B.Deaths;
    });

    for (const FRowEntry& R : Rows)
    {
        if (ULastFPSScoreRowWidget* RowW = CreateWidget<ULastFPSScoreRowWidget>(this, RowWidgetClass))
        {
            RowW->SetRow(R.PlayerName, R.Kills, R.Deaths, R.Assists, R.DamageDealt, R.DamageTaken, R.HealingReceived, R.bIsLocalPlayer);
            RowsContainer->AddChild(RowW);
        }
    }
}

void ULastFPSScoreboardWidget::StartAutoRefresh()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            AutoRefreshTimerHandle,
            this,
            &ULastFPSScoreboardWidget::RefreshScoreboard,
            FMath::Max(0.5f, AutoRefreshInterval),
            /*bLoop=*/true);
    }
}

void ULastFPSScoreboardWidget::StopAutoRefresh()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
    }
}

void ULastFPSScoreboardWidget::NativeDestruct()
{
    StopAutoRefresh();
    Super::NativeDestruct();
}
