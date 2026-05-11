#include "UI/LastFPSScoreboardWidget.h"
#include "Game/LastFPSMatchGameState.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/PlayerController.h"

namespace
{
struct FRowEntry
{
    FString PlayerName;
    int32   Kills           = 0;
    int32   Deaths          = 0;
    int32   Assists         = 0;
    int32   DamageDealt     = 0;
    bool    bIsLocalPlayer  = false;
};
}

void ULastFPSScoreboardWidget::RefreshScoreboard()
{
    // [TEMP DIAG] 실제 GameState 클래스명까지 출력 → BP가 GameStateClass를 다른 클래스로 오버라이드했는지 식별
    {
        AGameStateBase* AnyGS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
        UE_LOG(LogTemp, Warning,
            TEXT("[Scoreboard] Refresh — MatchResultText=%s ScoreboardText=%s | AnyGS=%s GSClass=%s"),
            MatchResultText ? TEXT("OK") : TEXT("NULL"),
            ScoreboardText  ? TEXT("OK") : TEXT("NULL"),
            AnyGS ? TEXT("OK") : TEXT("NULL"),
            AnyGS ? *AnyGS->GetClass()->GetName() : TEXT("-"));
    }

    const UWorld* World = GetWorld();
    const ALastFPSMatchGameState* MGS = World ? World->GetGameState<ALastFPSMatchGameState>() : nullptr;
    if (!MGS)
    {
        if (MatchResultText) MatchResultText->SetText(FText::GetEmpty());
        if (ScoreboardText)  ScoreboardText->SetText(FText::GetEmpty());
        return;
    }

    // 결과 헤더 — 매치 종료 시점에만 채워짐
    if (MatchResultText)
    {
        if (MGS->IsMatchEnded())
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

    if (!ScoreboardText)
    {
        return;
    }

    // 행 수집 → 정렬 → 단일 문자열로 결합
    const APlayerController* LocalPC = GetOwningPlayer();
    TArray<FRowEntry> Rows;
    Rows.Reserve(MGS->PlayerArray.Num());

    for (APlayerState* PS : MGS->PlayerArray)
    {
        const ALastFPSPlayerState* LFPS = Cast<ALastFPSPlayerState>(PS);
        if (!LFPS) continue;

        FRowEntry Row;
        Row.PlayerName     = LFPS->GetPlayerName();
        Row.Kills          = LFPS->GetStatKills();
        Row.Deaths         = LFPS->GetStatDeaths();
        Row.Assists        = LFPS->GetStatAssists();
        Row.DamageDealt    = FMath::RoundToInt(LFPS->GetStatDamageDealt());
        Row.bIsLocalPlayer = (LocalPC && LFPS == LocalPC->PlayerState);
        Rows.Add(Row);
    }

    Rows.Sort([](const FRowEntry& A, const FRowEntry& B)
    {
        return A.Kills != B.Kills ? A.Kills > B.Kills : A.Deaths < B.Deaths;
    });

    // Monospace 폰트 가정 — 컬럼 폭 고정
    FString Out = FString::Printf(TEXT("%-16s %3s %3s %3s %5s\n"),
        TEXT("Player"), TEXT("K"), TEXT("D"), TEXT("A"), TEXT("DMG"));

    for (const FRowEntry& R : Rows)
    {
        const TCHAR* Marker = R.bIsLocalPlayer ? TEXT("> ") : TEXT("  ");
        // 이름이 길면 14자로 자름 (Marker 2자 + 이름 14자 = 16자)
        FString Name = R.PlayerName.Left(14);
        Out += FString::Printf(TEXT("%s%-14s %3d %3d %3d %5d\n"),
            Marker, *Name, R.Kills, R.Deaths, R.Assists, R.DamageDealt);
    }

    ScoreboardText->SetText(FText::FromString(Out));
}
