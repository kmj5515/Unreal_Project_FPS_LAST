#include "UI/LastFPSUITags.h"

#include "GameplayTagsManager.h"

namespace LastFPSUITags
{
	static FGameplayTag RequestTag(const TCHAR* TagName)
	{
		return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
	}

	FGameplayTag Layer_Game()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Layer.Game"));
		return Tag;
	}

	FGameplayTag Layer_GameMenu()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Layer.GameMenu"));
		return Tag;
	}

	FGameplayTag Layer_Menu()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Layer.Menu"));
		return Tag;
	}

	FGameplayTag Layer_Modal()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Layer.Modal"));
		return Tag;
	}

	FGameplayTag Screen_Inventory()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Inventory"));
		return Tag;
	}

	FGameplayTag Screen_Mission()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Mission"));
		return Tag;
	}

	FGameplayTag Screen_Shop()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Shop"));
		return Tag;
	}

	FGameplayTag Screen_Settings()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Settings"));
		return Tag;
	}

	FGameplayTag Screen_Module()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Module"));
		return Tag;
	}
}
