#include "Game/Travel/LastFPSLevelTravelSettings.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include "Utility/LastFPSTravelTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLevelTravelSettings)

ULastFPSLevelTravelSettings::ULastFPSLevelTravelSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("LastFPS Level Travel");

	const FPrimaryAssetType& MapType = LastFPSPrimaryAssetTypes::Map;
	MainMenuMapId = FPrimaryAssetId(MapType, TEXT("/Game/Maps/IngameMap/MainMenuMap"));
	CharacterSelectMapId = FPrimaryAssetId(MapType, TEXT("/Game/Maps/IngameMap/CharacterSelectMap"));
	HubMapId = FPrimaryAssetId(MapType, TEXT("/Game/Maps/IngameMap/HubMap"));
}

const FPrimaryAssetId& ULastFPSLevelTravelSettings::GetMapId(
	const ELastFPSTravelDestination Destination) const
{
	switch (Destination)
	{
	case ELastFPSTravelDestination::MainMenu:
		return MainMenuMapId;
	case ELastFPSTravelDestination::CharacterSelect:
		return CharacterSelectMapId;
	case ELastFPSTravelDestination::Hub:
		return HubMapId;
	default:
		return MainMenuMapId;
	}
}
