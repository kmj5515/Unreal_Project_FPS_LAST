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
#include "ecsAnimationToolbarWidget.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Editor/UnrealEd/Public/SEditorViewport.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"

class SPropertyTreeViewImpl;
class IDetailsView;
class FecsCrosshairEditorModule;


class EASYCROSSHAIRSYSTEMEDITOR_API SecsCrosshairEditorWindow : public SCompoundWidget, public FEditorUndoClient
{
public:
    SLATE_BEGIN_ARGS(SecsCrosshairEditorWindow)
        : _Asset(nullptr)
    {}
    SLATE_ARGUMENT(UecsCrosshairEditorAsset*, Asset)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual ~SecsCrosshairEditorWindow();

	virtual bool SupportsKeyboardFocus() const override { return false; }
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	
    void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager);

    UecsCrosshairEditorAsset* GetEditingAsset() const { return EditingAsset; }
    TSharedPtr<SOverlay> ViewportOverlay;
    
    TSharedPtr<SecsCrosshairWidget>		CrosshairWidget;
    UecsCrosshairEditorAsset*					EditingAsset;
	TSharedPtr<SecsAnimationToolbarWidget>	ToolbarWidget;
	TSharedPtr<SImage>						CrosshairBackgroundImage;
	TSharedPtr<FSlateBrush>					CrosshairBackgroundBrush;
	TSharedPtr<SWidget>						PropertiesWidget;
private:
    void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
    TSharedRef<SWidget> CreatePropertiesWidget();
    void AddDynamicCrosshairWidgetToViewport();
    TSharedPtr<FAdvancedPreviewScene>			PreviewScene;
    TSharedPtr<IDetailsView>					DetailsView;
};
