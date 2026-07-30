#include "UI/HUD/Presenters/LastFPSEnemyHealthPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Character/LastFPSCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/HUD/LastFPSObjectiveHudSubsystem.h"
#include "Utility/LastFPSTags.h"

void ULastFPSEnemyHealthPresenter::Initialize(
    ULastFPSEnemyHealthBarWidget* InBossHealthBar,
    const FLastFPSEnemyHealthBarSettings& InSettings)
{
    BossHealthBar = InBossHealthBar;
    Settings = InSettings;
    ClearBossHealthBar();
}

void ULastFPSEnemyHealthPresenter::Shutdown()
{
    ClearEnemyHealthBars();
    ClearBossHealthBar();
}

void ULastFPSEnemyHealthPresenter::HandleDamage(
    APlayerController* OwningPlayer,
    AActor* DamageTargetActor,
    float DamageAmount)
{
    ALastFPSCharacterBase* Enemy = Cast<ALastFPSCharacterBase>(DamageTargetActor);
    ShowEnemyHealthBar(OwningPlayer, Enemy, DamageAmount);
}

void ULastFPSEnemyHealthPresenter::ShowEnemyHealthBar(
    APlayerController* OwningPlayer,
    ALastFPSCharacterBase* Enemy,
    float DamageAmount)
{
    if (!Enemy || Enemy->IsPlayerControlled() || !OwningPlayer || !Settings.WidgetClass)
    {
        return;
    }

    if (Enemy->HasCharacterClassificationTag(LastFPSGameplayTags::Character_Type_Boss))
    {
        // 보스 체력은 전용 HUD에서 표시하므로 일반 적 체력바를 사용하지 않는다.
        ReleaseEnemyHealthBarFor(Enemy);

        if (!Enemy->IsAlive())
        {
            ClearBossHealthBar();
            ReleaseBossPresentation();
        }
        else if (BossHealthBar && RequestBossPresentation())
        {
            // 슬롯을 얻었을 때만 띄운다 — InitializeForFixedHUDTarget 이 스스로 가시성을 켜므로
            // 허가 없이 부르면 점령·방어 표시 위에 겹쳐 뜬다.
            BossHealthBar->InitializeForFixedHUDTarget(Enemy, Settings, DamageAmount);
        }
        return;
    }

    if (!Enemy->IsAlive())
    {
        ReleaseEnemyHealthBarFor(Enemy);
        return;
    }

    for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
    {
        if (Widget && Widget->IsTrackingEnemy(Enemy))
        {
            Widget->RefreshDisplayDuration(Settings.DisplayDuration);
            Widget->NotifyDamage(DamageAmount, Settings);
            return;
        }
    }

    ULastFPSEnemyHealthBarWidget* SelectedWidget = nullptr;
    for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
    {
        if (Widget && Widget->IsAvailable())
        {
            SelectedWidget = Widget;
            break;
        }
    }

    const int32 MaxActiveBars = FMath::Max(Settings.MaxActiveBars, 1);
    if (!SelectedWidget && EnemyHealthBarPool.Num() < MaxActiveBars)
    {
        SelectedWidget = CreateWidget<ULastFPSEnemyHealthBarWidget>(OwningPlayer, Settings.WidgetClass);
        if (SelectedWidget)
        {
            SelectedWidget->AddToViewport(Settings.ViewportZOrder);
            EnemyHealthBarPool.Add(SelectedWidget);
        }
    }

    if (!SelectedWidget)
    {
        for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
        {
            if (Widget && (!SelectedWidget
                || Widget->GetRemainingDisplayTime() < SelectedWidget->GetRemainingDisplayTime()))
            {
                SelectedWidget = Widget;
            }
        }
    }

    if (SelectedWidget)
    {
        SelectedWidget->InitializeForEnemy(Enemy, Settings, DamageAmount);
    }
}

void ULastFPSEnemyHealthPresenter::ReleaseEnemyHealthBarFor(const ALastFPSCharacterBase* Enemy)
{
    if (!Enemy)
    {
        return;
    }

    for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
    {
        if (Widget && Widget->IsTrackingEnemy(Enemy))
        {
            Widget->ReleaseFromEnemy();
            return;
        }
    }
}

void ULastFPSEnemyHealthPresenter::Tick(float DeltaTime)
{
    for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
    {
        if (Widget && !Widget->IsAvailable())
        {
            Widget->UpdateTrackedEnemy(DeltaTime, Settings);
        }
    }

    if (BossHealthBar && !BossHealthBar->IsAvailable())
    {
        BossHealthBar->UpdateFixedHUDTarget(DeltaTime, Settings);
    }

    // 위젯이 스스로 대상을 놓는 경로(체력 0 복제 등)가 있어 여기서 슬롯 반납을 확인한다.
    ReleaseBossPresentationIfDetached();
}

void ULastFPSEnemyHealthPresenter::ClearEnemyHealthBars()
{
    for (ULastFPSEnemyHealthBarWidget* Widget : EnemyHealthBarPool)
    {
        if (Widget)
        {
            Widget->ReleaseFromEnemy();
            Widget->RemoveFromParent();
        }
    }
    EnemyHealthBarPool.Reset();
}

void ULastFPSEnemyHealthPresenter::ClearBossHealthBar()
{
    if (BossHealthBar)
    {
        BossHealthBar->ReleaseFromEnemy();
    }
    ReleaseBossPresentation();
}

bool ULastFPSEnemyHealthPresenter::RequestBossPresentation()
{
    if (bBossPresentationActive)
    {
        return true;
    }

    if (ULastFPSObjectiveHudSubsystem* Hud = GetHudSubsystem())
    {
        bBossPresentationActive =
            Hud->RequestPresentation(ELastFPSObjectiveHudMode::Boss, this);
    }
    return bBossPresentationActive;
}

void ULastFPSEnemyHealthPresenter::ReleaseBossPresentationIfDetached()
{
    if (!bBossPresentationActive)
    {
        return;
    }

    // 위젯이 대상을 놓았다면(보스 사망·대상 무효) 표시가 이미 끝난 것이다.
    if (!BossHealthBar || BossHealthBar->IsAvailable())
    {
        ReleaseBossPresentation();
    }
}

void ULastFPSEnemyHealthPresenter::ReleaseBossPresentation()
{
    if (!bBossPresentationActive)
    {
        return;
    }
    bBossPresentationActive = false;

    if (ULastFPSObjectiveHudSubsystem* Hud = GetHudSubsystem())
    {
        Hud->ReleasePresentation(this);
    }
}

ULastFPSObjectiveHudSubsystem* ULastFPSEnemyHealthPresenter::GetHudSubsystem() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<ULastFPSObjectiveHudSubsystem>() : nullptr;
}
