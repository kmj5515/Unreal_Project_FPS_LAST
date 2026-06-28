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
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "Animations/ecsCrosshairWidgetAnim.h"
#include "ecsCrosshairEditorAsset.generated.h"

class UecsCrosshairBehavior;

USTRUCT(BlueprintType)
struct FecsCrosshairLayerData
{
    GENERATED_BODY()
public:
    
    UPROPERTY(EditAnywhere, Category="Layer")
    TObjectPtr<UTexture2D> LayerTexture;
    
    UPROPERTY(EditAnywhere, Category="Layer")
    FLinearColor LayerColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category="Layer")
    FVector2D Transform = FVector2D(0.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category="Layer")
    float Rotation = 0.f;
    
    UPROPERTY(EditAnywhere, Category="Layer", meta=(ClampMin=-1.0, ClampMax=1.0))
    float Scale = 0.f;

    void SetLayerName(FName InLayerName)
    {
        LayerName = InLayerName;
    }
    
    FName       LayerName;
    
    // float       CurrentScale;
    // float       CurrentRotation = 0.f;
    // FVector2D   CurrentTransform;
 
    bool IsValid() const
    {
        return LayerTexture != nullptr;
    }
};

class UecsCrosshairBehavior;

/**
 * Custom asset for Dynamic Crosshair Editor
 * This asset will open the Dynamic Crosshair Editor window when double-clicked
 */
UCLASS(BlueprintType, hidecategories=(Object))
class EASYCROSSHAIRSYSTEM_API UecsCrosshairEditorAsset : public UObject
{
    GENERATED_BODY()

public:
    UecsCrosshairEditorAsset();

    UPROPERTY(EditAnywhere, Category="General")
    FString CrosshairName;
    
    UPROPERTY(EditAnywhere, Category="General")
    FText Description;
    
    UPROPERTY(EditAnywhere, Category="Layer")
    TMap<FString, FecsCrosshairLayerData> Layers;

    UPROPERTY(EditAnywhere, Category="Layer")
    float GlobalScale = 1.0f;
    
    UPROPERTY(EditAnywhere, Category="Animation")
    TArray<FecsCrosshairAnimation> Animations;

    UPROPERTY(EditAnywhere, Category="Behavior")
    TArray<UecsCrosshairBehavior*> Behaviors;

    UPROPERTY(EditAnywhere, Category="Editor")
    TObjectPtr<UTexture2D> EditorBackground;

    UPROPERTY(EditAnywhere, Category="Editor")
    float Zoom = 0.0f;
    
    FecsCrosshairAnimation GetAnimationByName(FName AnimationName) const
    {
        for (const auto& Animation : Animations)
        {
            if (Animation.Name == AnimationName)
            {
                return Animation;
            }
        }
        return FecsCrosshairAnimation();
    }
}; 