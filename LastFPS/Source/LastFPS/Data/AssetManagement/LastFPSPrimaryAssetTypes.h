#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"

namespace LastFPSPrimaryAssetTypes
{
	extern LASTFPS_API const FPrimaryAssetType BattleDefinition;
	extern LASTFPS_API const FPrimaryAssetType CharacterDefinition;
	extern LASTFPS_API const FPrimaryAssetType DestinationContentSet;
	extern LASTFPS_API const FPrimaryAssetType GameDataSet;
	extern LASTFPS_API const FPrimaryAssetType Map;
	extern LASTFPS_API const FPrimaryAssetType PopupCatalog;
	extern LASTFPS_API const FPrimaryAssetType WeaponDefinition;
}

namespace LastFPSAssetBundles
{
	extern LASTFPS_API const FName Startup;
	extern LASTFPS_API const FName Game;
	extern LASTFPS_API const FName UI;
	extern LASTFPS_API const FName Audio;
}
