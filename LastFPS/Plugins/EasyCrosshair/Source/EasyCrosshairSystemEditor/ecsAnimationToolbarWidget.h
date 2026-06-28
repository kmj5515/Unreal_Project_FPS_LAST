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
#include "Widgets/SCompoundWidget.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "AdvancedPreviewScene.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Editor/UnrealEd/Public/SEditorViewport.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "ecsAnimationToolbarWidget.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"

class SecsAnimationToolbarWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SecsAnimationToolbarWidget) {}
    SLATE_ARGUMENT(TSharedPtr<SecsCrosshairWidget>, CrosshairWidget)
    SLATE_ARGUMENT(UecsCrosshairEditorAsset*, EditingAsset)
SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    
    TSharedPtr<SecsCrosshairWidget> CrosshairWidget;
    UecsCrosshairEditorAsset*             EditingAsset;
    TSharedPtr<SOverlay>                Overlay;

    void SetEditingAsset(UecsCrosshairEditorAsset* InAsset) { EditingAsset = InAsset; }
    TSharedRef<SWidget> GenerateMenuContent();
private:

    TSharedPtr<STextBlock> TitleText;
    FName CurrentAnimationName = "None";
    void OnDefaultAnimationSelected();
    void PlayAnimationClicked(FecsCrosshairAnimation CrosshairAnimation);
};
