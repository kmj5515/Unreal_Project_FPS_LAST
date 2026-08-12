#include "AbilitySystem/Effects/GE_HealInstant.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"

namespace
{
    // 캐릭터별 MaxHealth 가 100~1,000,000 까지 벌어져 있어 고정 회복량은 의미가 유지되지 않는다.
    // 캐릭터마다 다른 비율이 필요해지면 이 상수를 BP 파생 GE 나 데이터 에셋으로 옮긴다.
    constexpr float HealMaxHealthRatio = 0.25f;
}

ULastFPSGE_HealInstant::ULastFPSGE_HealInstant()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FAttributeBasedFloat HealFromMaxHealth;
    HealFromMaxHealth.Coefficient = FScalableFloat(HealMaxHealthRatio);
    HealFromMaxHealth.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
    HealFromMaxHealth.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
        ULastFPSAttributeSet::GetMaxHealthAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        /*bSnapshot=*/false);

    FGameplayModifierInfo Mod;
    Mod.Attribute           = ULastFPSAttributeSet::GetHealthAttribute();
    Mod.ModifierOp          = EGameplayModOp::Additive;
    Mod.ModifierMagnitude   = FGameplayEffectModifierMagnitude(HealFromMaxHealth);
    Modifiers.Add(Mod);
}
