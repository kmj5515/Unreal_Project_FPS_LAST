// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsCrosshairEditorAsset.h"

UecsCrosshairEditorAsset::UecsCrosshairEditorAsset()
{
    // Set default values

    this->SetFlags(RF_Transactional);
    CrosshairName = TEXT("New Dynamic Crosshair");
    Description = FText::FromString(TEXT("Configure your dynamic crosshair settings"));

    // Foreach Layers and set LayerName for each TMap<FString, FecsCrosshairLayerData> Layers
    for (auto& Elem : Layers)
    {
        Elem.Value.SetLayerName(FName(*Elem.Key));
    }
    
} 