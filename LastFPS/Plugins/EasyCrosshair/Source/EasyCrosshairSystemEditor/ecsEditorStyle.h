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
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyleRegistry.h"

class FEasyCrosshairSystemEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();

    /** Get the style instance */
    static const ISlateStyle& Get();

    /** Get the style set name */
    static FName GetStyleSetName();

private:
    static TSharedPtr<FSlateStyleSet> StyleInstance;
};