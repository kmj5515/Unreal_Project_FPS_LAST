// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "ecsCrosshairEditorModule.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyleRegistry.h"

class FEasyCrosshairSystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    FecsCrosshairEditorModule& GetEditorModule() { return EditorModule; }
private:
    /** Editor module component */
    FecsCrosshairEditorModule EditorModule;
};
