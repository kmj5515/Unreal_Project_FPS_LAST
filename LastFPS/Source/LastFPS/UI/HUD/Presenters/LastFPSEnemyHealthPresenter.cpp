#include "UI/HUD/Presenters/LastFPSEnemyHealthPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Character/LastFPSCharacterBase.h"
#include "GameFramework/PlayerController.h"
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
        }
        else if (BossHealthBar)
        {
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
}
