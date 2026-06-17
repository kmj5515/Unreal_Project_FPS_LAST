#pragma once

#include "GameplayTagContainer.h"

/** UI.Layer.* gameplay tags used by CommonGame PrimaryGameLayout */
namespace LastFPSUITags
{
	LASTFPS_API FGameplayTag Layer_Game();
	LASTFPS_API FGameplayTag Layer_GameMenu();
	LASTFPS_API FGameplayTag Layer_Menu();
	LASTFPS_API FGameplayTag Layer_Modal();

	// UI.Screen.* — C++에서 참조하는 화면 태그 (나머지는 데이터/태그로만 사용)
	LASTFPS_API FGameplayTag Screen_Inventory();
	LASTFPS_API FGameplayTag Screen_Mission();
	LASTFPS_API FGameplayTag Screen_Shop();
	LASTFPS_API FGameplayTag Screen_Settings();
	LASTFPS_API FGameplayTag Screen_Module();
}
