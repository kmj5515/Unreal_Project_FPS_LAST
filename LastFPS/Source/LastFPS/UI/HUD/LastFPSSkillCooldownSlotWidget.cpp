#include "UI/HUD/LastFPSSkillCooldownSlotWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULastFPSSkillCooldownSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyConfiguredSkillIconBrush();
    ApplySkillIconTextureToMaterial();
    ApplyConfiguredKeyLabel();
}

void ULastFPSSkillCooldownSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyConfiguredSkillIconBrush();
    ApplySkillIconTextureToMaterial();
    ApplyConfiguredKeyLabel();

    if (SkillIcon && bDriveCooldownMaterial)
    {
        SkillIcon->GetDynamicMaterial();
    }

    InitializeSlotPresentation();
}

void ULastFPSSkillCooldownSlotWidget::ConfigureCooldownSlot(
    const FGameplayTag InCooldownTag,
    const TSubclassOf<UGameplayEffect> InCooldownEffectClass)
{
    DisplayMode = ELastFPSSkillSlotDisplayMode::Cooldown;
    CooldownTag = InCooldownTag;
    CooldownEffectClass = InCooldownEffectClass;
    InitializeSlotPresentation();
}

void ULastFPSSkillCooldownSlotWidget::InitializeSlotPresentation()
{
    SetVisibility(ESlateVisibility::HitTestInvisible);

    if (SkillIcon)
    {
        ApplyConfiguredSkillIconBrush();
        ApplySkillIconTextureToMaterial();

        SkillIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
        SkillIcon->SetRenderOpacity(IconReadyOpacity);

        if (bDriveCooldownMaterial && !CooldownMaterialParameterName.IsNone())
        {
            if (UMaterialInstanceDynamic* MID = SkillIcon->GetDynamicMaterial())
            {
                MID->SetScalarParameterValue(CooldownMaterialParameterName, 0.f);
            }
        }
    }

    if (CooldownText)
    {
        CooldownText->SetVisibility(ESlateVisibility::Collapsed);
        CooldownText->SetText(FText::GetEmpty());
    }

    if (ActiveOverlay)
    {
        ActiveOverlay->SetVisibility(
            bShowActiveOverlayWhenReady
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }

    if (KeyLabel)
    {
        ApplyConfiguredKeyLabel();
        KeyLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void ULastFPSSkillCooldownSlotWidget::SetSkillIconBrush(const FSlateBrush& Brush)
{
    SkillIconBrush = Brush;
    bUseConfiguredSkillIconBrush = true;
    ApplyConfiguredSkillIconBrush();
    ApplySkillIconTextureToMaterial();
    InitializeSlotPresentation();
}

void ULastFPSSkillCooldownSlotWidget::SetSkillIconTexture(UTexture2D* Texture, const bool bMatchSize)
{
    if (!Texture)
    {
        return;
    }

    SkillIconTexture = Texture;
    if (!bApplySkillIconTextureToMaterial)
    {
        SkillIconBrush.SetResourceObject(Texture);
        if (bMatchSize)
        {
            SkillIconBrush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
        }

        bUseConfiguredSkillIconBrush = true;
        ApplyConfiguredSkillIconBrush();
    }

    ApplySkillIconTextureToMaterial();
    InitializeSlotPresentation();
}

void ULastFPSSkillCooldownSlotWidget::SetSkillIconMaterial(UMaterialInterface* Material)
{
    if (!Material)
    {
        return;
    }

    SkillIconBrush.SetResourceObject(Material);
    bUseConfiguredSkillIconBrush = true;
    ApplyConfiguredSkillIconBrush();
    ApplySkillIconTextureToMaterial();
    InitializeSlotPresentation();
}

void ULastFPSSkillCooldownSlotWidget::SetKeyLabel(const FText& Label)
{
    if (bUseConfiguredKeyLabel)
    {
        ApplyConfiguredKeyLabel();
        return;
    }

    if (KeyLabel)
    {
        KeyLabel->SetText(Label);
    }
}

void ULastFPSSkillCooldownSlotWidget::SetConfiguredKeyLabel(const FText& Label)
{
    ConfiguredKeyLabel = Label;
    bUseConfiguredKeyLabel = true;
    ApplyConfiguredKeyLabel();
}

void ULastFPSSkillCooldownSlotWidget::ApplyConfiguredKeyLabel()
{
    if (KeyLabel && bUseConfiguredKeyLabel)
    {
        KeyLabel->SetText(ConfiguredKeyLabel);
    }
}

void ULastFPSSkillCooldownSlotWidget::ApplyConfiguredSkillIconBrush()
{
    if (SkillIcon && ShouldApplyConfiguredSkillIconBrush())
    {
        SkillIcon->SetBrush(SkillIconBrush);
    }
}

bool ULastFPSSkillCooldownSlotWidget::ShouldApplyConfiguredSkillIconBrush() const
{
    return bUseConfiguredSkillIconBrush || SkillIconBrush.GetResourceObject() != nullptr;
}

void ULastFPSSkillCooldownSlotWidget::ApplySkillIconTextureToMaterial()
{
    if (!SkillIcon || !bApplySkillIconTextureToMaterial || !SkillIconTexture || SkillIconTextureParameterName.IsNone())
    {
        return;
    }

    if (UMaterialInstanceDynamic* MID = SkillIcon->GetDynamicMaterial())
    {
        MID->SetTextureParameterValue(SkillIconTextureParameterName, SkillIconTexture);
    }
}

bool ULastFPSSkillCooldownSlotWidget::QueryCooldown(
    const UAbilitySystemComponent* ASC,
    const FGameplayTag Tag,
    const TSubclassOf<UGameplayEffect> InCooldownEffectClass,
    float& OutRemaining,
    float& OutDuration)
{
    OutRemaining = 0.f;
    OutDuration  = 0.f;

    if (!ASC || !Tag.IsValid())
    {
        return false;
    }

    FGameplayTagContainer Tags;
    Tags.AddTag(Tag);

    const FGameplayEffectQuery OwningQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags);
    const TArray<TPair<float, float>> OwningTimes = ASC->GetActiveEffectsTimeRemainingAndDuration(OwningQuery);

    const auto AbsorbTimes = [&OutRemaining, &OutDuration](const TArray<TPair<float, float>>& Times)
    {
        for (const TPair<float, float>& Entry : Times)
        {
            OutRemaining = FMath::Max(OutRemaining, Entry.Key);
            OutDuration  = FMath::Max(OutDuration, Entry.Value);
        }
    };

    if (!OwningTimes.IsEmpty())
    {
        AbsorbTimes(OwningTimes);
        return OutRemaining > KINDA_SMALL_NUMBER;
    }

    const UWorld* World = ASC->GetWorld();
    const float WorldTime = World ? World->GetTimeSeconds() : 0.f;

    for (const FActiveGameplayEffect& Active : &ASC->GetActiveGameplayEffects())
    {
        if (!Active.Spec.Def)
        {
            continue;
        }

        const bool bTagMatch = Tag.IsValid() && Active.Spec.Def->GetGrantedTags().HasTag(Tag);
        const bool bClassMatch = InCooldownEffectClass
            && Active.Spec.Def->GetClass() == InCooldownEffectClass;
        if (!bTagMatch && !bClassMatch)
        {
            continue;
        }

        const float Remaining = Active.GetTimeRemaining(WorldTime);
        const float Duration  = Active.GetDuration();
        OutRemaining = FMath::Max(OutRemaining, Remaining);
        OutDuration  = FMath::Max(OutDuration, Duration);
    }

    return OutRemaining > KINDA_SMALL_NUMBER;
}

void ULastFPSSkillCooldownSlotWidget::ApplyVisual(
    const bool bReady,
    const float CooldownFraction,
    const float TimeRemaining)
{
    if (SkillIcon && bDriveCooldownMaterial && !CooldownMaterialParameterName.IsNone())
    {
        if (UMaterialInstanceDynamic* MID = SkillIcon->GetDynamicMaterial())
        {
            const float Clamped = FMath::Clamp(CooldownFraction, 0.f, 1.f);
            const float Value   = bReady
                ? 0.f
                : (bCooldownMaterialUsesRemainingFraction ? Clamped : (1.f - Clamped));
            MID->SetScalarParameterValue(CooldownMaterialParameterName, Value);
            ApplySkillIconTextureToMaterial();
        }
    }

    if (SkillIcon)
    {
        SkillIcon->SetRenderOpacity(bReady ? IconReadyOpacity : IconBlockedOpacity);
    }

    if (ActiveOverlay)
    {
        ActiveOverlay->SetVisibility(
            bShowActiveOverlayWhenReady && bReady
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
    }

    if (CooldownText)
    {
        if (bReady || TimeRemaining <= KINDA_SMALL_NUMBER)
        {
            CooldownText->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
            CooldownText->SetText(
                FText::AsNumber(FMath::Max(1, FMath::CeilToInt(TimeRemaining))));
        }
    }

    OnSkillSlotStateChanged(bReady, CooldownFraction, TimeRemaining);
}

void ULastFPSSkillCooldownSlotWidget::UpdateFromASC(
    const UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        ApplyVisual(true, 0.f, 0.f);
        return;
    }

    float Remaining = 0.f;
    float Duration  = 0.f;
    if (!QueryCooldown(ASC, CooldownTag, CooldownEffectClass, Remaining, Duration))
    {
        ApplyVisual(true, 0.f, 0.f);
        return;
    }

    const bool bReady = Remaining <= KINDA_SMALL_NUMBER;
    const float Fraction = (Duration > KINDA_SMALL_NUMBER) ? (Remaining / Duration) : 0.f;
    ApplyVisual(bReady, Fraction, Remaining);
}
