#pragma once

#include "CoreMinimal.h"

class FLastFPSStartMapPlayService
{
public:
	static bool CanPlayConfiguredStartMap();
	static void PlayConfiguredStartMap();

private:
	static FString GetConfiguredStartMapPackageName();
	static bool IsPackageUsableForPIE(const FString& PackageName);
	static void ShowError(const FText& Message);
};
