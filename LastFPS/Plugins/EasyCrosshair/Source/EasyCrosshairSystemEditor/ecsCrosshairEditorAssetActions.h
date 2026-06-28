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
#include "AssetTypeActions_Base.h"

class UecsCrosshairEditorAsset;

/**
 * Asset type actions for DCrosshairEditorAsset
 * This handles operations such as opening the asset when double-clicked
 */
class EASYCROSSHAIRSYSTEMEDITOR_API FecsCrosshairEditorAssetActions : public FAssetTypeActions_Base
{
public:
    FecsCrosshairEditorAssetActions();

    // FAssetTypeActions_Base interface
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override { return EAssetTypeCategories::UI; }
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
    // End of FAssetTypeActions_Base interface
    virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override;
    virtual const FSlateBrush* GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const override;
    
    // Thumbnail
    virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;
    
}; 