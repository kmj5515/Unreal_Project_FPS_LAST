// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "EasyCrosshairSystem.h"

#define LOCTEXT_NAMESPACE "FEasyCrosshairSystemModule"

void FEasyCrosshairSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FEasyCrosshairSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEasyCrosshairSystemModule, EasyCrosshairSystem)