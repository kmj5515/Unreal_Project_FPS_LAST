#include "Game/LastFPSGameUserSettings.h"

ULastFPSGameUserSettings* ULastFPSGameUserSettings::Get()
{
	return Cast<ULastFPSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}
