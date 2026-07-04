#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "UI/Framework/LastFPSUITags.h"

#include "CommonActivatableWidget.h"
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

// 커서 소유 판정에 쓰는 "메뉴류" 레이어 순서 — Game(HUD) 은 제외한다. Index 는 이 순서를 따른다.
static FGameplayTag MenuLikeLayerTag(int32 Index)
{
	switch (Index)
	{
	case 0:  return LastFPSUITags::Layer_Modal();
	case 1:  return LastFPSUITags::Layer_Menu();
	case 2:  return LastFPSUITags::Layer_GameMenu();
	default: return FGameplayTag();
	}
}

void ULastFPSPrimaryGameLayout::RestoreFocusToTopActiveWidget()
{
	// Game(HUD) 레이어 제외 — 메뉴가 다 닫히면 ESC 가 PC(ESC 메뉴)로 흐르도록.
	for (int32 i = 0; i < NumMenuLikeLayers; ++i)
	{
		if (const UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(MenuLikeLayerTag(i)))
		{
			if (UCommonActivatableWidget* Active = Layer->GetActiveWidget())
			{
				Active->SetIsFocusable(true);
				Active->SetKeyboardFocus();
				return;
			}
		}
	}
}

bool ULastFPSPrimaryGameLayout::HasActiveMenuLikeWidget()
{
	for (int32 i = 0; i < NumMenuLikeLayers; ++i)
	{
		if (const UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(MenuLikeLayerTag(i)))
		{
			if (Layer->GetActiveWidget() != nullptr)
			{
				return true;
			}
		}
	}
	return false;
}

UCommonActivatableWidgetContainerBase* ULastFPSPrimaryGameLayout::GetMenuLikeLayerContainer(int32 Index)
{
	if (Index < 0 || Index >= NumMenuLikeLayers)
	{
		return nullptr;
	}
	return GetLayerWidget(MenuLikeLayerTag(Index));
}
