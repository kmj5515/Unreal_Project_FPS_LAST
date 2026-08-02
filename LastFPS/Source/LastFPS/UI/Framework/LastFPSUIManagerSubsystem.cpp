#include "UI/Framework/LastFPSUIManagerSubsystem.h"

#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "UI/Framework/LastFPSActivatableWidget.h"
#include "UI/Framework/LastFPSScreenRegistry.h"
#include "UI/Framework/LastFPSUITags.h"

#include "CommonActivatableWidget.h"
#include "CommonLocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSUIManagerSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSUI, Log, All);

void ULastFPSUIManagerSubsystem::Deinitialize()
{
	// 부모 종료가 UI 정책을 해제하기 전에 로컬 플레이어별 루트 레이아웃을 Viewport와 정책 캐시에서 제거한다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		const int32 LocalPlayerCount = GameInstance->GetNumLocalPlayers();
		for (int32 PlayerIndex = 0; PlayerIndex < LocalPlayerCount; ++PlayerIndex)
		{
			if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(GameInstance->GetLocalPlayerByIndex(PlayerIndex)))
			{
				NotifyPlayerDestroyed(LocalPlayer);
			}
		}
	}

	OpenScreens.Reset();
	ScreenTabHost.Reset();
	CachedRegistry = nullptr;

	Super::Deinitialize();
}

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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULastFPSGameDataSubsystem* GameData = GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>())
		{
			CachedRegistry = GameData->GetScreenRegistry();
		}
	}

	if (!CachedRegistry)
	{
		UE_LOG(LogLastFPSUI, Error, TEXT("Startup GameDataSet에서 ScreenRegistry를 찾지 못했습니다."));
	}
	return CachedRegistry;
}

UCommonActivatableWidget* ULastFPSUIManagerSubsystem::OpenScreen(FGameplayTag ScreenTag, APlayerController* OwningPlayer)
{
	return OpenScreenWithInit(ScreenTag, OwningPlayer, nullptr);
}

UCommonActivatableWidget* ULastFPSUIManagerSubsystem::OpenScreenWithInit(
	FGameplayTag ScreenTag,
	APlayerController* OwningPlayer,
	TFunction<void(UCommonActivatableWidget&)> InitFunc)
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

	// 표시 방식은 데이터가 정한다. 껍데기가 떠 있는지 여부로 갈리면 같은 호출이 상황마다 다르게 동작한다.
	if (Def->Presentation == ELastFPSScreenPresentation::ShellTab)
	{
		if (ILastFPSScreenTabHost* TabHost = Cast<ILastFPSScreenTabHost>(ScreenTabHost.Get()))
		{
			if (TabHost->HostsScreenTab(ScreenTag))
			{
				return TabHost->ShowScreenTab(ScreenTag);
			}
		}

		if (!Def->HostScreenTag.IsValid())
		{
			UE_LOG(LogLastFPSUI, Error,
				TEXT("'%s' 는 ShellTab 인데 HostScreenTag 가 비어 있어 열 곳이 없습니다."),
				*ScreenTag.ToString());
			return nullptr;
		}

		// 껍데기를 먼저 연다. 껍데기가 같은 프레임에 준비되지 않을 수 있어 요청을 남겨 두고,
		// 탭 호스트가 등록되는 시점에 이어서 적용한다.
		PendingTabScreenTag = ScreenTag;
		return OpenScreen(Def->HostScreenTag, OwningPlayer);
	}

	// 여기부터는 레이어 push 경로다.

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

	// 초기화는 construct 전에 넘겨야 화면이 첫 프레임부터 제 내용을 그린다.
	UCommonActivatableWidget* Widget = RootLayout->PushWidgetToLayerStack<UCommonActivatableWidget>(
		LayerTag,
		WidgetClass,
		[&InitFunc](UCommonActivatableWidget& PushedWidget)
		{
			if (InitFunc)
			{
				InitFunc(PushedWidget);
			}
		});

	if (Widget)
	{
		OpenScreens.Add(ScreenTag, Widget);
	}
	return Widget;
}

void ULastFPSUIManagerSubsystem::RegisterScreenTabHost(TScriptInterface<ILastFPSScreenTabHost> InTabHost)
{
	if (!InTabHost)
	{
		return;
	}

	if (ScreenTabHost.IsValid() && ScreenTabHost.Get() != InTabHost.GetObject())
	{
		// shell 이 둘 이상 살아 있으면 어느 쪽으로 위임할지 결정할 근거가 없다. 조용히 덮지 않고 알린다.
		UE_LOG(LogLastFPSUI, Warning,
			TEXT("탭 shell 이 이미 등록돼 있습니다(%s). %s 로 교체합니다."),
			*GetNameSafe(ScreenTabHost.Get()), *GetNameSafe(InTabHost.GetObject()));
	}

	ScreenTabHost = InTabHost.GetObject();

	// 여기서 바로 적용하지 않는다. 등록은 껍데기가 탭 목록을 만들기 전에 일어나므로
	// HostsScreenTab 이 아직 false 라 요청이 그대로 버려진다.
	// 껍데기가 목록을 채운 뒤 ConsumePendingTabScreenTag 로 직접 가져가게 한다.
}

FGameplayTag ULastFPSUIManagerSubsystem::ConsumePendingTabScreenTag()
{
	const FGameplayTag PendingTag = PendingTabScreenTag;
	PendingTabScreenTag = FGameplayTag();
	return PendingTag;
}

void ULastFPSUIManagerSubsystem::UnregisterScreenTabHost(const TScriptInterface<ILastFPSScreenTabHost>& InTabHost)
{
	// 이미 다른 shell 이 등록된 뒤 늦게 들어온 해제 요청이 그것을 지우지 않게 한다.
	if (InTabHost && ScreenTabHost.Get() == InTabHost.GetObject())
	{
		ScreenTabHost.Reset();
	}
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
