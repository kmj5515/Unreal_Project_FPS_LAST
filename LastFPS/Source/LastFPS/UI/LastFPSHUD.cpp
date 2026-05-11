#include "UI/LastFPSHUD.h"
#include "UI/LastFPSHUDWidget.h"
#include "UI/LastFPSScoreboardWidget.h"
#include "Game/LastFPSMatchGameState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ALastFPSHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();

    if (HUDWidgetClass)
    {
        HUDWidget = CreateWidget<ULastFPSHUDWidget>(PC, HUDWidgetClass);
        if (HUDWidget)
            HUDWidget->AddToViewport(0);
    }

    if (ScoreboardWidgetClass)
    {
        ScoreboardWidget = CreateWidget<ULastFPSScoreboardWidget>(PC, ScoreboardWidgetClass);
        if (ScoreboardWidget)
        {
            ScoreboardWidget->AddToViewport(1);
            ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    TryBindMatchGameState();
}

void ALastFPSHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
    }

    if (ALastFPSMatchGameState* MGS = BoundMatchGameState.Get())
    {
        MGS->OnMatchEnded.Remove(MatchEndedHandle);
    }
    BoundMatchGameState.Reset();
    MatchEndedHandle.Reset();

    Super::EndPlay(EndPlayReason);
}

void ALastFPSHUD::TryBindMatchGameState()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>();
    if (!MGS)
    {
        // 클라에서 GameState 복제가 아직 안 된 경우 — 0.1초마다 재시도
        World->GetTimerManager().SetTimer(
            BindRetryTimerHandle, this, &ALastFPSHUD::RetryBindMatchGameState, 0.1f, true);
        return;
    }

    BoundMatchGameState = MGS;
    MatchEndedHandle    = MGS->OnMatchEnded.AddUObject(this, &ALastFPSHUD::HandleMatchEnded);

    // 늦게 바인딩되어 이미 매치 종료 상태면 즉시 1회 처리
    if (MGS->IsMatchEnded())
    {
        HandleMatchEnded();
    }
}

void ALastFPSHUD::RetryBindMatchGameState()
{
    if (BoundMatchGameState.IsValid()) return;

    if (UWorld* World = GetWorld())
    {
        if (ALastFPSMatchGameState* MGS = World->GetGameState<ALastFPSMatchGameState>())
        {
            World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
            BoundMatchGameState = MGS;
            MatchEndedHandle    = MGS->OnMatchEnded.AddUObject(this, &ALastFPSHUD::HandleMatchEnded);
            if (MGS->IsMatchEnded())
            {
                HandleMatchEnded();
            }
        }
    }
}

void ALastFPSHUD::HandleMatchEnded()
{
    ShowScoreboard();

    // 결과 화면 동안 게임 입력 차단 — 매핑을 비우면 모든 IA가 무효화됨
    APlayerController* PC = GetOwningPlayerController();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    if (UEnhancedInputLocalPlayerSubsystem* Sub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
    {
        Sub->ClearAllMappings();
    }
}

void ALastFPSHUD::ShowHitMarker()
{
    if (HUDWidget)
        HUDWidget->ShowHitMarker();
}

void ALastFPSHUD::ShowScoreboard()
{
    if (!ScoreboardWidget) return;
    ScoreboardWidget->RefreshScoreboard();
    ScoreboardWidget->SetVisibility(ESlateVisibility::Visible);
}

void ALastFPSHUD::HideScoreboard()
{
    if (ScoreboardWidget)
        ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
}
