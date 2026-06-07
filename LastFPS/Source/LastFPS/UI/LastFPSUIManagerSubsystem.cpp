#include "UI/LastFPSUIManagerSubsystem.h"

#include "UI/LastFPSActivatableWidget.h"
#include "UI/LastFPSScreenRegistry.h"
#include "UI/LastFPSUISettings.h"
#include "UI/LastFPSUITags.h"

#include "CommonActivatableWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSUIManagerSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSUI, Log, All);

ULastFPSUIManagerSubsystem* ULastFPSUIManagerSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<ULastFPSUIManagerSubsystem>();
		}
	}
	return nullptr;
}

const ULastFPSScreenRegistry* ULastFPSUIManagerSubsystem::GetRegistry()
{
	if (CachedRegistry)
	{
		return CachedRegistry;
	}

	const ULastFPSUISettings* Settings = GetDefault<ULastFPSUISettings>();
	if (Settings)
	{
		CachedRegistry = Settings->ScreenRegistry.LoadSynchronous();
	}

	if (!CachedRegistry)
	{
		UE_LOG(LogLastFPSUI, Error,
			TEXT("ScreenRegistry 미지정 — Project Settings > Game > LastFPS UI 에서 설정 필요"));
	}
	return CachedRegistry;
}

UCommonActivatableWidget* ULastFPSUIManagerSubsystem::OpenScreen(FGameplayTag ScreenTag, APlayerController* OwningPlayer)
{
	if (!ScreenTag.IsValid())
	{
		return nullptr;
	}

	// 이미 열려 있으면 그대로 반환.
	if (TWeakObjectPtr<UCommonActivatableWidget>* Existing = OpenScreens.Find(ScreenTag))
	{
		if (Existing->IsValid() && Existing->Get()->IsActivated())
		{
			return Existing->Get();
		}
		OpenScreens.Remove(ScreenTag);
	}

	const ULastFPSScreenRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return nullptr;
	}

	const FLastFPSScreenDef* Def = Registry->FindScreen(ScreenTag);
	if (!Def)
	{
		UE_LOG(LogLastFPSUI, Error, TEXT("ScreenRegistry에 '%s' 정의 없음"), *ScreenTag.ToString());
		return nullptr;
	}

	APlayerController* PC = OwningPlayer ? OwningPlayer : GetGameInstance()->GetFirstLocalPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(PC);
	if (!RootLayout)
	{
		return nullptr;
	}

	TSubclassOf<UCommonActivatableWidget> WidgetClass = Def->WidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogLastFPSUI, Error, TEXT("'%s' 의 WidgetClass 로드 실패"), *ScreenTag.ToString());
		return nullptr;
	}

	const FGameplayTag LayerTag = Def->LayerTag.IsValid() ? Def->LayerTag : LastFPSUITags::Layer_Menu();

	UCommonActivatableWidget* Widget = RootLayout->PushWidgetToLayerStack<UCommonActivatableWidget>(LayerTag, WidgetClass);
	if (Widget)
	{
		OpenScreens.Add(ScreenTag, Widget);
	}
	return Widget;
}

void ULastFPSUIManagerSubsystem::CloseScreen(FGameplayTag ScreenTag)
{
	if (TWeakObjectPtr<UCommonActivatableWidget>* Found = OpenScreens.Find(ScreenTag))
	{
		if (Found->IsValid())
		{
			Found->Get()->DeactivateWidget();
		}
		OpenScreens.Remove(ScreenTag);
	}
}

bool ULastFPSUIManagerSubsystem::IsScreenOpen(FGameplayTag ScreenTag) const
{
	const TWeakObjectPtr<UCommonActivatableWidget>* Found = OpenScreens.Find(ScreenTag);
	return Found && Found->IsValid() && Found->Get()->IsActivated();
}
