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
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "Blueprint/UserWidget.h"
#include "Animations/ecsCrosshairWidgetAnim.h"
#include "Curves/CurveVector.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Images/SImage.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "ecsCrosshairWidget.generated.h"

class UecsCrosshairBehavior;

class EASYCROSSHAIRSYSTEM_API SecsCrosshairWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SecsCrosshairWidget)
		{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	void SetLayerTransform(FName LayerName, FVector2D Transform, float Rotation, float Scale);
	void ResetLayerTransform(FName LayerName);
	
	void SetLayerColor(FName LayerName, const FLinearColor& InColor); // TODO: This is additive, rename
	void ResetLayerColor(FName LayerName);
	void SetLayerColorDirect(FName LayerName, const FLinearColor& InColor);

	void PlayAnimation(FecsCrosshairAnimation Animation);

	void StopAllAnimations(bool bGracefully = true);
	void StopAnimation(FecsCrosshairAnimation Animation);

	TMap<FString, FecsCrosshairLayerData>	Layers;
	TArray<TSharedPtr<FSlateBrush>>			Brushes;
	TArray<TSharedPtr<SImage>>				Images;
	
	bool IsLayersExist(TArray<FName> Layers);
	
	FecsCrosshairLayerData&	GetLayerByName(FName LayerName);
	TSharedPtr<SImage>		GetLayerImageByName(FName LayerName) const;
	TSharedPtr<FSlateBrush> GetLayerBrushByName(FName LayerName) const;
	void StopAllAnimationsExcept(const TArray<FName>& ExcludedLayers, bool bGracefully);
	void SetCrosshairLayers(const TMap<FString, FecsCrosshairLayerData>& InLayers);
	void SetCrosshairBehaviors(const TArray<UecsCrosshairBehavior*>& InBehaviors, UecsCrosshairWidget* Widget);

	float GlobalScale;
	
	UPROPERTY()
	TArray<UecsCrosshairBehavior*> 					Behaviors;
	TArray<float>						 			BehaviorTickTimers;
	TArray<TObjectPtr<UecsCrosshairAnimationLayer>>	ActiveAnimationLayers;


private:
	void StopAnimationLayer(UecsCrosshairAnimationLayer* AnimLayer, bool bGracefully = true);
	FSlateRenderTransform BuildLayerTransform(FVector2D Pivot, float Scale, float Rotation, FVector2D Translation);
	
	TArray<float> OriginalLayerScales;
	TArray<int32> AnimatedLayerIndices;
	TSharedPtr<SOverlay> Overlay;
};

/**
 * 
 */
UCLASS()
class EASYCROSSHAIRSYSTEM_API UecsCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	UFUNCTION(BlueprintCallable, Category = "EasyCrosshair")
	void ConfigureCrosshairWidget(UecsCrosshairEditorAsset* InEditorAsset);

	
	TSharedPtr<class SecsCrosshairWidget> CrosshairWidget;

	UecsCrosshairEditorAsset * GetCrosshairEditorAsset() const
    {
        return CrosshairEditorAsset;
    }
private:
	UPROPERTY()
	UecsCrosshairEditorAsset* CrosshairEditorAsset;
};