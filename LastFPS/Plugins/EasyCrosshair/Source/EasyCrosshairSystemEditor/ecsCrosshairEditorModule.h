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
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorToolkit.h"

class FecsCrosshairEditorAssetActions;
class SDockTab;
class FSpawnTabArgs;
class UecsCrosshairEditorAsset;


class EASYCROSSHAIRSYSTEMEDITOR_API FecsCrosshairEditorModule
{
public:
    /** Constructor */
    FecsCrosshairEditorModule();
    
    /** Destructor */
    ~FecsCrosshairEditorModule();

    /** Initializes the module component */
    void Initialize();

    /** Shuts down the module component */
    void Shutdown();

    /** Opens a new Crosshair editor window */
    void OpenCrosshairEditorWindow(UecsCrosshairEditorAsset* Asset);

    /** Returns the current asset being edited (could be null) */
    UecsCrosshairEditorAsset* GetEditingAsset() const { return EditingAsset; }

private:
    /** Creates a new Crosshair editor tab */
    TSharedRef<SDockTab> CreateCrosshairEditorTab(const FSpawnTabArgs& Args);

    /** Registers asset types */
    void RegisterAssetTypes();

    /** Unregisters asset types */
    void UnregisterAssetTypes();

private:
    /** The asset type actions for our custom editor asset */
    TSharedPtr<FecsCrosshairEditorAssetActions> CrosshairEditorAssetActions;

    /** Stores the asset currently being edited (if any) */
    UecsCrosshairEditorAsset* EditingAsset = nullptr;
    
    /** Flag to determine if this module component has been initialized */
    bool bInitialized = false;
}; 