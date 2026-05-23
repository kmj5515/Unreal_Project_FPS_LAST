// Copyright Epic Games, Inc. All Rights Reserved.

#include "LastFPS.h"
#include "Modules/ModuleManager.h"
#include "Utility/LastFPSTags.h"

class FLastFPSModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FLastFPSTags::InitializeNativeTags();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FLastFPSModule, LastFPS, "LastFPS" );
