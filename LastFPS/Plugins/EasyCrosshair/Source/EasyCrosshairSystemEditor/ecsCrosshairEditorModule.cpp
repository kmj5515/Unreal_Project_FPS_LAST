// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com
#include "ecsCrosshairEditorModule.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "ecsCrosshairEditorAssetActions.h"
#include "AssetToolsModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IAssetTools.h"
#include "PropertyEditorModule.h"
#include "ecsCrosshairEditorWindow.h"
#include "ecsEditorStyle.h"

#define LOCTEXT_NAMESPACE "DCrosshairEditor"

// Define tab identifiers
static const FName DCrosshairEditorTabId = FName(TEXT("CrosshairEditorTab"));

FecsCrosshairEditorModule::FecsCrosshairEditorModule()
    : EditingAsset(nullptr)
    , bInitialized(false)
{
}

FecsCrosshairEditorModule::~FecsCrosshairEditorModule()
{
    // Make sure we're properly cleaned up
    if (bInitialized)
    {
        Shutdown();
    }
}

void FecsCrosshairEditorModule::Initialize()
{
    if (!bInitialized)
    {
        FEasyCrosshairSystemEditorStyle::Initialize();
        
        // Register asset types
        RegisterAssetTypes();
        bInitialized = true;
    }
}

void FecsCrosshairEditorModule::Shutdown()
{
    if (bInitialized)
    {
        // Unregister the tab spawner
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DCrosshairEditorTabId);

        // Unregister asset types
        UnregisterAssetTypes();
        
        bInitialized = false;
    }
}



void FecsCrosshairEditorModule::RegisterAssetTypes()
{
    // Get the asset tools module
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    // Register in the User Interface category
    EAssetTypeCategories::Type UICategory = AssetTools.RegisterAdvancedAssetCategory(
        FName(TEXT("EasyCrosshair")), 
        LOCTEXT("EasyCrosshairCategory", "Easy Crosshair")
    );

    // Create and register the asset type actions
    CrosshairEditorAssetActions = MakeShared<FecsCrosshairEditorAssetActions>();
    
    AssetTools.RegisterAssetTypeActions(CrosshairEditorAssetActions.ToSharedRef());
}

void FecsCrosshairEditorModule::UnregisterAssetTypes()
{
    // Unregister the asset type actions
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        if (CrosshairEditorAssetActions.IsValid())
        {
            AssetTools.UnregisterAssetTypeActions(CrosshairEditorAssetActions.ToSharedRef());
            CrosshairEditorAssetActions.Reset();
        }
    }
}

// Opens a new Crosshair editor window
void FecsCrosshairEditorModule::OpenCrosshairEditorWindow(UecsCrosshairEditorAsset* Asset)
{
    FString AssetName = Asset ? Asset->GetName() : TEXT("None");

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        DCrosshairEditorTabId,
        FOnSpawnTab::CreateRaw(this, &FecsCrosshairEditorModule::CreateCrosshairEditorTab))
    .SetDisplayName(FText::FromString(AssetName))
    .SetMenuType(ETabSpawnerMenuType::Hidden)
    .SetIcon(FSlateIcon(FEasyCrosshairSystemEditorStyle::GetStyleSetName(), "EasyCrosshairSystemEditor.Icon"));
    
    // Store the asset in a member variable so it's accessible by the tab
    EditingAsset = Asset;

    // Try to invoke or bring the tab to the front if it already exists
    FGlobalTabmanager::Get()->TryInvokeTab(DCrosshairEditorTabId);
}

// Create the editor tab with the split layout
TSharedRef<SDockTab> FecsCrosshairEditorModule::CreateCrosshairEditorTab(const FSpawnTabArgs& Args)
{
    // Print EditingAsset->CrosshairName
    if (EditingAsset)
    {
        TSharedRef<SDockTab> Tab = SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SecsCrosshairEditorWindow)
                .Asset(EditingAsset)
            ];
            return Tab;
    }
    else
    {
        // Return null if no asset is set
        TSharedRef<SDockTab> Tab = SNew(SDockTab);
        // Remove the tab from the tab manager
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DCrosshairEditorTabId);
        return Tab;
    }

}

#undef LOCTEXT_NAMESPACE 