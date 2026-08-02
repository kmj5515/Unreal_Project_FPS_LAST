#include "UI/Theme/LastFPSUITheme.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/Theme/LastFPSUISettings.h"
#include "UI/Theme/LastFPSUIThemeAsset.h"
#include "UI/Theme/LastFPSUIThemeReceiver.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSUITheme, Log, All);

namespace
{
	/** M_UI_SlotBorder 가 노출하는 파라미터 이름 — 머티리얼과 코드가 맞춰야 하는 계약이라 한 곳에 모은다. */
	const FName SlotBorderTintParam(TEXT("TintColor"));
	const FName SlotBorderGlowParam(TEXT("GlowIntensity"));

	/** Root 아래를 훑으며 인터페이스 구현 위젯에 테마를 넘긴다. UserWidget 은 자신의 트리로 이어서 내려간다. */
	void ApplyRecursive(UWidget* Widget, const ULastFPSUIThemeAsset& Theme, int32& OutAppliedCount)
	{
		if (!Widget)
		{
			return;
		}

		if (ILastFPSUIThemeReceiver* Receiver = Cast<ILastFPSUIThemeReceiver>(Widget))
		{
			Receiver->ApplyUITheme(Theme);
			++OutAppliedCount;
		}

		if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
		{
			if (UserWidget->WidgetTree)
			{
				ApplyRecursive(UserWidget->WidgetTree->RootWidget, Theme, OutAppliedCount);
			}
			return;
		}

		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const int32 ChildCount = Panel->GetChildrenCount();
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				ApplyRecursive(Panel->GetChildAt(ChildIndex), Theme, OutAppliedCount);
			}
		}
	}
}

const ULastFPSUIThemeAsset* LastFPSUITheme::GetActiveTheme()
{
	const ULastFPSUISettings* Settings = ULastFPSUISettings::Get();
	if (!Settings || Settings->ActiveTheme.IsNull())
	{
		return nullptr;
	}

	// 화면 진입 시 1회 호출이므로 동기 로드로 충분하다. 이후에는 소프트 포인터가 캐시를 돌려준다.
	return Settings->ActiveTheme.LoadSynchronous();
}

int32 LastFPSUITheme::ApplyToWidgetTree(UWidget* Root)
{
	if (!Root)
	{
		return 0;
	}

	const ULastFPSUIThemeAsset* Theme = GetActiveTheme();
	if (!Theme)
	{
		UE_LOG(LogLastFPSUITheme, Warning,
			TEXT("%s: 활성 UI 테마가 없습니다. 프로젝트 설정 > Game > LastFPS UI 에서 ActiveTheme 을 지정하세요."),
			*Root->GetName());
		return 0;
	}

	int32 AppliedCount = 0;
	ApplyRecursive(Root, *Theme, AppliedCount);
	return AppliedCount;
}

void LastFPSUITheme::ApplyRarityGlow(
	UImage& BorderImage, const FLinearColor& RarityColor, const float GlowIntensity)
{
	if (Cast<UMaterialInterface>(BorderImage.GetBrush().GetResourceObject()))
	{
		// 동적 인스턴스는 이미지마다 1개만 만들어지고 이후 호출에서는 같은 것이 돌아온다.
		if (UMaterialInstanceDynamic* DynamicMaterial = BorderImage.GetDynamicMaterial())
		{
			DynamicMaterial->SetVectorParameterValue(SlotBorderTintParam, RarityColor);
			DynamicMaterial->SetScalarParameterValue(SlotBorderGlowParam, GlowIntensity);
			BorderImage.SetColorAndOpacity(FLinearColor::White);
			return;
		}
	}

	BorderImage.SetColorAndOpacity(RarityColor * FMath::Max(GlowIntensity, 0.f));
}
