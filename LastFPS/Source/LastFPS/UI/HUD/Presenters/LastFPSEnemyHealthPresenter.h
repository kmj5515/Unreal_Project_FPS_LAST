#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/HUD/LastFPSEnemyHealthBarWidget.h"
#include "LastFPSEnemyHealthPresenter.generated.h"

class AActor;
class APlayerController;
class ALastFPSCharacterBase;

/**
 * 적/보스 체력바 표시를 HUD View에서 분리한다.
 * 피해를 준 적만 제한된 수의 위젯 풀로 표시하고, 보스는 전용 고정 체력바로 표시한다.
 * 다수의 적이 있어도 UI 비용이 일정하게 유지되도록 풀을 재사용한다.
 */
UCLASS()
class LASTFPS_API ULastFPSEnemyHealthPresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 보스 전용 체력바(선택적 바인딩 위젯, null 허용)와 표시 설정을 받아 구성한다. */
    void Initialize(ULastFPSEnemyHealthBarWidget* InBossHealthBar, const FLastFPSEnemyHealthBarSettings& InSettings);

    /** 풀과 보스 체력바를 모두 정리한다. View의 NativeDestruct에서 호출한다. */
    void Shutdown();

    /** 피해 이벤트를 받아 대상에 맞는 체력바(일반/보스)를 갱신하거나 새로 표시한다. */
    void HandleDamage(APlayerController* OwningPlayer, AActor* DamageTargetActor, float DamageAmount);

    /** 표시 중인 체력바들의 위치·잔여 시간을 매 HUD 갱신 주기마다 갱신한다. */
    void Tick(float DeltaTime);

private:
    void ShowEnemyHealthBar(APlayerController* OwningPlayer, ALastFPSCharacterBase* Enemy, float DamageAmount);
    void ReleaseEnemyHealthBarFor(const ALastFPSCharacterBase* Enemy);
    void ClearEnemyHealthBars();
    void ClearBossHealthBar();

    UPROPERTY(Transient)
    TArray<TObjectPtr<ULastFPSEnemyHealthBarWidget>> EnemyHealthBarPool;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSEnemyHealthBarWidget> BossHealthBar;

    UPROPERTY()
    FLastFPSEnemyHealthBarSettings Settings;
};
