// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsCrosshairEditorWindow.h"
#include "AdvancedPreviewScene.h"
#include "Editor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SSplitter.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "FileHelpers.h"


#define LOCTEXT_NAMESPACE "DCrosshairEditor"

void SecsCrosshairEditorWindow::Construct(const FArguments& InArgs)
{
    // Initialize Editing Asset
    EditingAsset = InArgs._Asset;
    GEditor->RegisterForUndo(this);

    if (!EditingAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("No EditingAsset provided."));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("EditingAsset: %s"), *EditingAsset->CrosshairName);

    // Initialize Widgets
    CrosshairWidget = SNew(SecsCrosshairWidget);
    CrosshairWidget->GlobalScale = EditingAsset->GlobalScale + EditingAsset->Zoom;
    CrosshairWidget->SetCrosshairLayers(EditingAsset->Layers);

    PropertiesWidget = CreatePropertiesWidget();
    
    ToolbarWidget->CrosshairWidget = CrosshairWidget;

    // Setup background brush
    CrosshairBackgroundBrush = MakeShared<FSlateBrush>();
    if (EditingAsset->EditorBackground)
    {
        CrosshairBackgroundBrush->SetResourceObject(EditingAsset->EditorBackground);
        CrosshairBackgroundBrush->DrawAs = ESlateBrushDrawType::Image;
        CrosshairBackgroundBrush->Tiling = ESlateBrushTileType::NoTile;
    }

    // Create background image
    CrosshairBackgroundImage = SNew(SImage)
        .Image(CrosshairBackgroundBrush.Get());

    // Create overlay for crosshair view
    TSharedRef<SOverlay> CrosshairOverlay = SNew(SOverlay)
    + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
    [
        SNew(SBox)
        .WidthOverride(1024.f)
        .HeightOverride(1024.f)
        [
            CrosshairBackgroundImage.ToSharedRef()
        ]
    ]
    + SOverlay::Slot()
    [
        CrosshairWidget.ToSharedRef()
    ];

    // Properties panel


    // Main layout
    ChildSlot
    [
        SNew(SSplitter)
        .Orientation(Orient_Horizontal)
        + SSplitter::Slot()
        .Value(0.5f)
        [
            CrosshairOverlay
        ]
        + SSplitter::Slot()
        .Value(0.5f)
        [
            PropertiesWidget.ToSharedRef()
        ]
    ];

    // Set the object in the details view
    if (DetailsView.IsValid())
    {
        DetailsView->SetObject(EditingAsset);
    }

    ToolbarWidget->SetEditingAsset(EditingAsset);
}


SecsCrosshairEditorWindow::~SecsCrosshairEditorWindow()
{
}

void SecsCrosshairEditorWindow::PostUndo(bool bSuccess)
{
    if (
        CrosshairWidget
        && EditingAsset
        && ToolbarWidget
        )
    {
        PropertiesWidget = CreatePropertiesWidget();
        CrosshairWidget->GlobalScale = EditingAsset->GlobalScale + EditingAsset->Zoom;
        CrosshairWidget->SetCrosshairLayers(EditingAsset->Layers);
        
        // Toolbar Widget
        ToolbarWidget->CrosshairWidget = CrosshairWidget;
        ToolbarWidget->SetEditingAsset(EditingAsset);

        if (EditingAsset->EditorBackground)
        {
            CrosshairBackgroundBrush->SetResourceObject(EditingAsset->EditorBackground);
            CrosshairBackgroundBrush->DrawAs = ESlateBrushDrawType::Type::Image;
            CrosshairBackgroundBrush->Tiling = ESlateBrushTileType::Type::NoTile;
        }
    }
}

void SecsCrosshairEditorWindow::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (
        CrosshairWidget
        && EditingAsset
        && ToolbarWidget
        )
    {
        PropertiesWidget = CreatePropertiesWidget();
        CrosshairWidget->GlobalScale = EditingAsset->GlobalScale + EditingAsset->Zoom;
        CrosshairWidget->SetCrosshairLayers(EditingAsset->Layers);
        
        // Toolbar Widget
        ToolbarWidget->CrosshairWidget = CrosshairWidget;
        ToolbarWidget->SetEditingAsset(EditingAsset);

        if (EditingAsset->EditorBackground)
        {
            CrosshairBackgroundBrush->SetResourceObject(EditingAsset->EditorBackground);
            CrosshairBackgroundBrush->DrawAs = ESlateBrushDrawType::Type::Image;
            CrosshairBackgroundBrush->Tiling = ESlateBrushTileType::Type::NoTile;
        }
    }
}

TSharedRef<SWidget> SecsCrosshairEditorWindow::CreatePropertiesWidget()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bUpdatesFromSelection = true;
    DetailsViewArgs.bLockable = false;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bHideSelectionTip = true;
    
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->OnFinishedChangingProperties().AddSP(this, &SecsCrosshairEditorWindow::OnFinishedChangingProperties);

    ToolbarWidget = SNew(SecsAnimationToolbarWidget)
        .CrosshairWidget(CrosshairWidget)
        .EditingAsset(EditingAsset);
    
    // Create the properties panel
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PropertiesHeader", "Crosshair Properties"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(2)
        [
            DetailsView.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            ToolbarWidget.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SButton)
            .Text(LOCTEXT("SaveButton", "Save Crosshair"))
            .OnClicked_Lambda([this]() { 
                // Save the asset if it has an outer package
                if (EditingAsset && EditingAsset->GetOutermost() != GetTransientPackage())
                {
                    TArray<UPackage*> PackagesToSave;
                    PackagesToSave.Add(EditingAsset->GetOutermost());
                    FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
                }
                return FReply::Handled(); 
            })
        ];
}

void SecsCrosshairEditorWindow::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    // Not implemented for now - for future tab management
}

void SecsCrosshairEditorWindow::AddDynamicCrosshairWidgetToViewport()
{
    // Get the game world from the preview scene
    UWorld* World = PreviewScene->GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot add Dynamic Crosshair Widget: World is not valid"));
        return;
    }
 

}

#undef LOCTEXT_NAMESPACE 