#include "UI/HUD/LastFPSStatusEffectIconWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"

void ULastFPSStatusEffectIconWidget::NativeDestruct()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	Super::NativeDestruct();
}

void ULastFPSStatusEffectIconWidget::InitializeStatusEffect(
	const FLastFPSStatusEffectUIData& InData)
{
	StatusEffectData = InData;
	SetToolTipText(InData.Description.IsEmpty() ? InData.DisplayName : InData.Description);

	if (StatusIcon)
	{
		if (UMaterialInstanceDynamic* MID = StatusIcon->GetDynamicMaterial())
		{
			MID->SetScalarParameterValue(CooldownMaterialParameterName, 0.f);
		}
	}
	RequestIconTexture();

	if (StackText)
	{
		StackText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULastFPSStatusEffectIconWidget::RequestIconTexture()
{
	if (IconLoadHandle.IsValid())
	{
		IconLoadHandle->CancelHandle();
		IconLoadHandle.Reset();
	}

	if (UTexture2D* LoadedTexture = StatusEffectData.Icon.Get())
	{
		ApplyIconTexture(LoadedTexture);
		return;
	}

	const FSoftObjectPath IconPath = StatusEffectData.Icon.ToSoftObjectPath();
	if (!IconPath.IsValid())
	{
		return;
	}

	IconLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		IconPath,
		FStreamableDelegate::CreateUObject(this, &ULastFPSStatusEffectIconWidget::HandleIconTextureLoaded),
		FStreamableManager::AsyncLoadHighPriority);
}

void ULastFPSStatusEffectIconWidget::HandleIconTextureLoaded()
{
	ApplyIconTexture(StatusEffectData.Icon.Get());
	IconLoadHandle.Reset();
}

void ULastFPSStatusEffectIconWidget::ApplyIconTexture(UTexture2D* IconTexture) const
{
	if (!StatusIcon || !IconTexture)
	{
		return;
	}

	if (UMaterialInstanceDynamic* MID = StatusIcon->GetDynamicMaterial())
	{
		MID->SetTextureParameterValue(StatusIconTextureParameterName, IconTexture);
	}
}

void ULastFPSStatusEffectIconWidget::UpdateRuntimeState(
	const float TimeRemaining,
	const float Duration,
	const int32 StackCount)
{
	const bool bFiniteDuration = Duration > KINDA_SMALL_NUMBER && TimeRemaining >= 0.f;
	if (StatusIcon)
	{
		if (UMaterialInstanceDynamic* MID = StatusIcon->GetDynamicMaterial())
		{
			float Value = bFiniteDuration ? FMath::Clamp(TimeRemaining / Duration, 0.f, 1.f) : 1.f;
			MID->SetScalarParameterValue(CooldownMaterialParameterName, Value);
		}
	}

	if (StackText)
	{
		const bool bShowStack = StatusEffectData.bShowStackCount && StackCount > 1;
		StackText->SetVisibility(
			bShowStack ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		StackText->SetText(bShowStack ? FText::AsNumber(StackCount) : FText::GetEmpty());
	}
}
