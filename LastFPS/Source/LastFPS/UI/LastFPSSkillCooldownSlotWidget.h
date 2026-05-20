#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "LastFPSSkillCooldownSlotWidget.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UImage;
class UTextBlock;

UENUM(BlueprintType)
enum class ELastFPSSkillSlotDisplayMode : uint8
{
    Cooldown,
    UltimateCharge
};

/**
 * WBP_SkillCooldownSlot 의 Parent.
 * Designer: SkillIcon(Brush=MI_000 등), CooldownText, KeyLabel
 */
UCLASS()
class LASTFPS_API ULastFPSSkillCooldownSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void ConfigureCooldownSlot(FGameplayTag InCooldownTag, TSubclassOf<UGameplayEffect> InCooldownEffectClass);
    void ConfigureUltimateSlot();
    /** HUD에서 슬롯 표시 직전 1회 호출 — 머티리얼·Opacity·텍스트 기본 상태 */
    void InitializeSlotPresentation();
    void UpdateFromASC(const UAbilitySystemComponent* ASC, const class ULastFPSAttributeSet* AttributeSetOverride = nullptr);

    UFUNCTION(BlueprintCallable, Category="HUD|Skill")
    void SetKeyLabel(const FText& Label);

protected:
    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill")
    ELastFPSSkillSlotDisplayMode DisplayMode = ELastFPSSkillSlotDisplayMode::Cooldown;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill")
    FGameplayTag CooldownTag;

    UPROPERTY()
    TSubclassOf<UGameplayEffect> CooldownEffectClass;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<UImage> SkillIcon;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> CooldownText;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Skill", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> KeyLabel;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill")
    float IconReadyOpacity = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill")
    float IconBlockedOpacity = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill|Material")
    bool bDriveCooldownMaterial = true;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill|Material")
    FName CooldownMaterialParameterName = TEXT("CoolDownRemainingPercent");

    UPROPERTY(EditDefaultsOnly, Category="HUD|Skill|Material")
    bool bCooldownMaterialUsesRemainingFraction = true;

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|Skill")
    void OnSkillSlotStateChanged(bool bReady, float CooldownPercent, float TimeRemaining);

private:
    void ApplyVisual(bool bReady, float CooldownFraction, float TimeRemaining);

    static bool QueryCooldown(
        const UAbilitySystemComponent* ASC,
        FGameplayTag Tag,
        TSubclassOf<UGameplayEffect> InCooldownEffectClass,
        float& OutRemaining,
        float& OutDuration);
};
