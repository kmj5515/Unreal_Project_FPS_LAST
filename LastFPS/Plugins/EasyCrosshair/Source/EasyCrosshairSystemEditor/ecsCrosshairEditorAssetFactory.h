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
#include "Factories/Factory.h"
#include "ecsCrosshairEditorAssetFactory.generated.h"

/**
 * Factory for creating DCrosshairEditorAsset objects
 */
UCLASS()
class EASYCROSSHAIRSYSTEMEDITOR_API UecsCrosshairEditorAssetFactory : public UFactory
{
    GENERATED_BODY()

public:
    UecsCrosshairEditorAssetFactory();
    virtual FName GetNewAssetThumbnailOverride() const override;
    // UFactory Interface
    
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool ShouldShowInNewMenu() const override;
    // End of UFactory Interface
}; 