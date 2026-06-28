// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsCrosshairEditorAssetFactory.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "AssetTypeCategories.h"

UecsCrosshairEditorAssetFactory::UecsCrosshairEditorAssetFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UecsCrosshairEditorAsset::StaticClass();
}

FName UecsCrosshairEditorAssetFactory::GetNewAssetThumbnailOverride() const
{
    return FName(TEXT("EasyCrosshairSystemEditor.Icon"));
}

UObject* UecsCrosshairEditorAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UecsCrosshairEditorAsset* NewAsset = NewObject<UecsCrosshairEditorAsset>(InParent, InClass, InName, Flags);
    return NewAsset;
}

bool UecsCrosshairEditorAssetFactory::ShouldShowInNewMenu() const
{
    return true;
} 