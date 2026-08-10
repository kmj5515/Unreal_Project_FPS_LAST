#include "UI/Framework/LastFPSPopupTags.h"

#include "GameplayTagsManager.h"

namespace LastFPSPopupTags
{
	static FGameplayTag RequestTag(const TCHAR* TagName)
	{
		return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
	}

	FGameplayTag Confirmation()
	{
		static const FGameplayTag Tag =
			RequestTag(TEXT("UI.Popup.Confirmation"));
		return Tag;
	}

	FGameplayTag Notice()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Popup.Notice"));
		return Tag;
	}

	FGameplayTag Quantity()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Popup.Quantity"));
		return Tag;
	}

	FGameplayTag Dialogue()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Popup.Dialogue"));
		return Tag;
	}

	FGameplayTag MissionResult()
	{
		static const FGameplayTag Tag = RequestTag(TEXT("UI.Popup.MissionResult"));
		return Tag;
	}
}
