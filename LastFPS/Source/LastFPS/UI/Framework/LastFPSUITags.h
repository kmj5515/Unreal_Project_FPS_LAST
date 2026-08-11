#pragma once

#include "GameplayTagContainer.h"

/** UI.Layer.* gameplay tags used by CommonGame PrimaryGameLayout */
namespace LastFPSUITags
{
	LASTFPS_API FGameplayTag Layer_Game();
	LASTFPS_API FGameplayTag Layer_GameMenu();
	LASTFPS_API FGameplayTag Layer_Menu();
	LASTFPS_API FGameplayTag Layer_Modal();
	LASTFPS_API FGameplayTag Layer_Overlay();

	// UI.Screen.* — C++에서 참조하는 화면 태그 (나머지는 데이터/태그로만 사용)
	LASTFPS_API FGameplayTag Screen_GameMenu();
	LASTFPS_API FGameplayTag Screen_Inventory();
	LASTFPS_API FGameplayTag Screen_Consumable();
	LASTFPS_API FGameplayTag Screen_Mission();
	LASTFPS_API FGameplayTag Screen_Shop();
	LASTFPS_API FGameplayTag Screen_Settings();
	LASTFPS_API FGameplayTag Screen_Module();
	LASTFPS_API FGameplayTag Screen_Equipment();
	LASTFPS_API FGameplayTag Screen_Map();
	LASTFPS_API FGameplayTag Screen_WeaponPreview();

	// UI.Preview.Slot.* — 프리뷰 무대에서 대상이 서는 자리. 무대 BP 의 슬롯에 같은 태그를 단다.
	LASTFPS_API FGameplayTag PreviewSlot_Character();
	LASTFPS_API FGameplayTag PreviewSlot_Weapon();

	// UI.Preview.View.* — 프리뷰 무대의 시점. 무대 BP 의 카메라에 같은 태그를 단다.
	LASTFPS_API FGameplayTag PreviewView_Character();
	LASTFPS_API FGameplayTag PreviewView_Weapon();
}
