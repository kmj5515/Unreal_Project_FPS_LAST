#include "Settings/MW_Settings.h"

#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "UMW_Settings"

UMW_Settings::UMW_Settings()
{
	ForcedPlayStartMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Test/MainMenuMap.MainMenuMap")));
}

#if WITH_EDITOR
FText UMW_Settings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Editor Tools");
}

FText UMW_Settings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Configure LastFPS editor tools.");
}
#endif

#undef LOCTEXT_NAMESPACE
