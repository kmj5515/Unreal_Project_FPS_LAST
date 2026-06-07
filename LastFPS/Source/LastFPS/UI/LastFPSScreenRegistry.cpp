#include "UI/LastFPSScreenRegistry.h"

const FLastFPSScreenDef* ULastFPSScreenRegistry::FindScreen(const FGameplayTag& ScreenTag) const
{
	return Screens.Find(ScreenTag);
}
