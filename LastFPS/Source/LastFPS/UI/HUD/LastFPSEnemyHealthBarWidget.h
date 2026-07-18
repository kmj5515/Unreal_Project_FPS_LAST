#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "LastFPSEnemyHealthBarWidget.generated.h"

class ULastFPSStatusEffectListWidget;
class ALastFPSCharacterBase;
class UAbilitySystemComponent;
class ULastFPSEnemyHealthBarWidget;
class UProgressBar;
class UTextBlock;

USTRUCT(BlueprintType)
struct FLastFPSEnemyHealthBarSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar")
    TSubclassOf<ULastFPSEnemyHealthBarWidget> WidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar", meta=(ClampMin="1", ClampMax="32"))
    int32 MaxActiveBars = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar", meta=(ClampMin="0.1", Units="s"))
    float DisplayDuration = 3.f;

    /** 피해 구간을 그대로 보여준 뒤 감소를 시작하기까지의 시간이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar|Damage Trail", meta=(ClampMin="0.0", Units="s"))
    float DamageTrailHoldDuration = 0.2f;

    /** 피해 구간이 현재 체력까지 완전히 감소하는 데 걸리는 시간이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar|Damage Trail", meta=(ClampMin="0.01", Units="s"))
    float DamageTrailDrainDuration = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar|Damage Trail")
    FLinearColor DamageTrailFillColor = FLinearColor(1.f, 0.78f, 0.18f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar", meta=(ClampMin="0.0", Units="cm"))
    float MaxDisplayDistance = 3000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar")
    FVector WorldOffset = FVector(0.f, 0.f, 110.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar")
    FVector2D ScreenOffset = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar", meta=(ClampMin="0.0"))
    float ScreenVisibilityPadding = 24.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Health Bar")
    int32 ViewportZOrder = 10;
};

/** HUD가 풀링하여 사용하는 화면 공간 적 체력 표시다. 자체 Tick 없이 HUD 갱신 주기를 공유한다. */
UCLASS()
class LASTFPS_API ULastFPSEnemyHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULastFPSEnemyHealthBarWidget(const FObjectInitializer& ObjectInitializer);

    void InitializeForEnemy(
        ALastFPSCharacterBase* Enemy,
        const FLastFPSEnemyHealthBarSettings& Settings,
        float InitialDamageAmount);
    /** HUD에 고정된 전용 체력바용 초기화다. 월드 위치 추적과 자동 숨김을 사용하지 않는다. */
    void InitializeForFixedHUDTarget(
        ALastFPSCharacterBase* Enemy,
        const FLastFPSEnemyHealthBarSettings& Settings,
        float InitialDamageAmount);
    void NotifyDamage(float DamageAmount, const FLastFPSEnemyHealthBarSettings& Settings);
    void RefreshDisplayDuration(float DisplayDuration);
    void ReleaseFromEnemy();
    bool UpdateTrackedEnemy(float DeltaTime, const FLastFPSEnemyHealthBarSettings& Settings);
    bool UpdateFixedHUDTarget(float DeltaTime, const FLastFPSEnemyHealthBarSettings& Settings);

    bool IsTrackingEnemy(const ALastFPSCharacterBase* Enemy) const;
    bool IsAvailable() const { return !TrackedEnemy.IsValid(); }
    float GetRemainingDisplayTime() const { return RemainingDisplayTime; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintImplementableEvent, Category="Enemy Health Bar")
    void OnEnemyHealthChanged(float InCurrentHealth, float InMaxHealth, const FText& DisplayName);

    UPROPERTY(BlueprintReadOnly, Category="Enemy Health Bar", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_EnemyHealth;

    /** 현재 HP 뒤에 배치되어 방금 잃은 체력 구간을 보여주는 보조 게이지다. */
    UPROPERTY(BlueprintReadOnly, Category="Enemy Health Bar", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> PB_EnemyHealthDamageTrail;

    UPROPERTY(BlueprintReadOnly, Category="Enemy Health Bar", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> TXT_EnemyName;
    
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<ULastFPSStatusEffectListWidget> WBP_StatusEffectList;
private:
    void EnsureNativeWidgets();
    void BindToEnemy(ALastFPSCharacterBase* Enemy);
    void UnbindFromEnemy();
    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
    void RefreshHealthDisplay();
    void UpdateDamageTrail(float DeltaTime, const FLastFPSEnemyHealthBarSettings& Settings);
    void ApplyHealthBars();
    bool UpdateScreenPosition(const FLastFPSEnemyHealthBarSettings& Settings);
    FText ResolveDisplayName() const;

    TWeakObjectPtr<ALastFPSCharacterBase> TrackedEnemy;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle HealthDelegateHandle;
    FDelegateHandle MaxHealthDelegateHandle;
    float RemainingDisplayTime = 0.f;
    float CurrentHealth = 0.f;
    float CurrentMaxHealth = 1.f;
    float DamageTrailHealth = 0.f;
    float DamageTrailHoldRemaining = 0.f;
};
