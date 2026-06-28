// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "EasyCrosshairSystemEditor.h"

#include "ecsEditorStyle.h"

#define LOCTEXT_NAMESPACE "FEasyCrosshairSystemEditorModule"

void FEasyCrosshairSystemEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
#if WITH_EDITOR
	// Initialize the editor module component
	EditorModule.Initialize();

#endif
}

void FEasyCrosshairSystemEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
#if WITH_EDITOR
	// Clean up the editor module component
	EditorModule.Shutdown();
#endif
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FEasyCrosshairSystemEditorModule, EasyCrosshairSystemEditor)