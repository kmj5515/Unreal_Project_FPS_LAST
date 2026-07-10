#include "Settings/LastFPSBattleLevelSettings.h"

#define LOCTEXT_NAMESPACE "ULastFPSBattleLevelSettings"

ULastFPSBattleLevelSettings::ULastFPSBattleLevelSettings()
{
	BattleLevelRootPath.Path = TEXT("/Game/Maps/Battle");
	RequiredActorTags.Add(TEXT("BattleStart"));
	RequiredActorTags.Add(TEXT("EnemySpawn"));
}

#if WITH_EDITOR
FText ULastFPSBattleLevelSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Battle Levels");
}

FText ULastFPSBattleLevelSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Configure LastFPS battle level editor tools.");
}
#endif

#undef LOCTEXT_NAMESPACE
