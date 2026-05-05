#include "UI/LastFPSScoreboardWidget.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/GameStateBase.h"

void ULastFPSScoreboardWidget::RefreshScoreboard()
{
    TArray<FPlayerScoreRow> Rows;

    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS)
    {
        OnScoreboardRefreshed(Rows);
        return;
    }

    const APlayerController* LocalPC = GetOwningPlayer();

    for (APlayerState* PS : GS->PlayerArray)
    {
        const ALastFPSPlayerState* LFPS = Cast<ALastFPSPlayerState>(PS);
        if (!LFPS) continue;

        FPlayerScoreRow Row;
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

    // 킬 내림차순 → 데스 오름차순 정렬
    Rows.Sort([](const FPlayerScoreRow& A, const FPlayerScoreRow& B)
    {
        return A.Kills != B.Kills ? A.Kills > B.Kills : A.Deaths < B.Deaths;
    });

    OnScoreboardRefreshed(Rows);
}
