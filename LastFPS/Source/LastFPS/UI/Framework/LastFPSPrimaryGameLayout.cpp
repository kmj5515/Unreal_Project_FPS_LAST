#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "UI/Framework/LastFPSUITags.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void ULastFPSPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureLayersRegistered();
}

void ULastFPSPrimaryGameLayout::EnsureLayersRegistered()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	auto RegisterStackLayer = [this](
		TObjectPtr<UCommonActivatableWidgetContainerBase>& AutoSlot,
		UCommonActivatableWidgetContainerBase* BoundSlot,
		const FGameplayTag& LayerTag)
	{
		if (!LayerTag.IsValid() || GetLayerWidget(LayerTag))
		{
			return;
		}

		UCommonActivatableWidgetContainerBase* Container = BoundSlot;
		if (!Container)
		{
			if (!AutoSlot)
			{
				AutoSlot = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
					UCommonActivatableWidgetStack::StaticClass());
				if (RootCanvas && AutoSlot)
				{
					if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(AutoSlot))
					{
						Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
						Slot->SetOffsets(FMargin(0.f));
					}
				}
			}
			Container = AutoSlot;
		}

		if (Container)
		{
			RegisterLayer(LayerTag, Container);
		}
	};

	RegisterStackLayer(AutoLayer_Game, LayerStack_Game, LastFPSUITags::Layer_Game());
	RegisterStackLayer(AutoLayer_GameMenu, LayerStack_GameMenu, LastFPSUITags::Layer_GameMenu());
	RegisterStackLayer(AutoLayer_Menu, LayerStack_Menu, LastFPSUITags::Layer_Menu());
	RegisterStackLayer(AutoLayer_Modal, LayerStack_Modal, LastFPSUITags::Layer_Modal());
}

ULastFPSPrimaryGameLayout::ULastFPSPrimaryGameLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
