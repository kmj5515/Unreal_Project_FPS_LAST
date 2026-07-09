#include "Settings/EUW_Settings.h"

#include "Engine/World.h"

UEUW_Settings::UEUW_Settings()
{
	ForcedPlayStartMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Test/MainMenuMap.MainMenuMap")));
}
