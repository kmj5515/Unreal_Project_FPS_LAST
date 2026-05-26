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
}
