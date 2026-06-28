// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsCrosshairEditorAssetActions.h"
#include "EasyCrosshairSystem//ecsCrosshairEditorAsset.h"
#include "ecsCrosshairEditorModule.h"
#include "EasyCrosshairSystemEditor.h"
#include "ecsEditorStyle.h"
#include "EasyCrosshairSystem/EasyCrosshairSystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"

// Constructor - initialize to UI category by default, but allow the module to override
FecsCrosshairEditorAssetActions::FecsCrosshairEditorAssetActions()
{
}

// Returns the name of this type
FText FecsCrosshairEditorAssetActions::GetName() const
{
    return NSLOCTEXT("AssetTypeActions", "ecsCrosshairEditorAssetActions", "Easy Crosshair");
}

// Returns the color associated with this type
FColor FecsCrosshairEditorAssetActions::GetTypeColor() const
{
    return FColor::Green;
}

// Returns the class this asset actions is for
UClass* FecsCrosshairEditorAssetActions::GetSupportedClass() const
{
    return UecsCrosshairEditorAsset::StaticClass();
}

// Opens the asset editor when double-clicked
void FecsCrosshairEditorAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    // Get the main plugin module first, then access its editor component
    FEasyCrosshairSystemEditorModule& PluginModule = FModuleManager::LoadModuleChecked<FEasyCrosshairSystemEditorModule>("EasyCrosshairSystemEditor");
    FecsCrosshairEditorModule& CrosshairEditorModule = PluginModule.GetEditorModule();

    for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
    {
        UecsCrosshairEditorAsset* Asset = Cast<UecsCrosshairEditorAsset>(*ObjIt);
        if (Asset != nullptr)
        {
            CrosshairEditorModule.OpenCrosshairEditorWindow(Asset);
        }
    }
}

const FSlateBrush* FecsCrosshairEditorAssetActions::GetThumbnailBrush(const FAssetData& InAssetData,
    const FName InClassName) const
{
    return FEasyCrosshairSystemEditorStyle::Get().GetBrush("EasyCrosshairSystemEditor.Icon");
}

const FSlateBrush* FecsCrosshairEditorAssetActions::GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const
{
    return FEasyCrosshairSystemEditorStyle::Get().GetBrush("EasyCrosshairSystemEditor.Icon");
}


void FecsCrosshairEditorAssetActions::GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section)
{
    FAssetTypeActions_Base::GetActions(InObjects, Section);
} 