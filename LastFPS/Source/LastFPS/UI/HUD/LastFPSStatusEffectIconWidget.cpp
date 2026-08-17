#include "UI/HUD/LastFPSStatusEffectIconWidget.h"

#include "Components/Border.h"
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

	if (CategoryBackground)
	{
		// 브러시 크기는 WBP 저작값을 유지한다(bMatchSize=false). 텍스처 스트리밍은 UImage가 처리한다.
		// IsNull() 은 경로 유무만 보므로 로드 여부는 보장하지 않는다. Get() 으로 읽으면 아직 로드되지
		// 않은 정상 경로에서 nullptr 브러시가 되어, 카테고리 색(버프=초록) 대신 흰 사각형이 남는다.
		// ponytail: 프레임 텍스처가 작아 동기 로드로 둔다. 히칭이 보이면 아이콘처럼 비동기 핸들로 옮길 것.
		if (const TSoftObjectPtr<UTexture2D>* CategoryFrame = CategoryBackgroundTextures.Find(InData.Category);
			CategoryFrame && !CategoryFrame->IsNull())
		{
			if (UTexture2D* CategoryTexture = CategoryFrame->LoadSynchronous())
			{
				CategoryBackground->SetBrushFromTexture(CategoryTexture);
			}
		}
	}

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
