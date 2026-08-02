#include "UI/Framework/LastFPSUITags.h"

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

	FGameplayTag Layer_Overlay()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Layer.Overlay"));
		return Tag;
	}

	FGameplayTag Screen_Inventory()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Inventory"));
		return Tag;
	}

	FGameplayTag Screen_Consumable()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Consumable"));
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

	FGameplayTag Screen_Equipment()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Equipment"));
		return Tag;
	}

	FGameplayTag Screen_WeaponPreview()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.WeaponPreview"));
		return Tag;
	}

	FGameplayTag PreviewSlot_Character()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Preview.Slot.Character"));
		return Tag;
	}

	FGameplayTag PreviewSlot_Weapon()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Preview.Slot.Weapon"));
		return Tag;
	}

	FGameplayTag PreviewView_Character()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Preview.View.Character"));
		return Tag;
	}

	FGameplayTag PreviewView_Weapon()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Preview.View.Weapon"));
		return Tag;
	}

	FGameplayTag Screen_Map()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Screen.Map"));
		return Tag;
	}
}
