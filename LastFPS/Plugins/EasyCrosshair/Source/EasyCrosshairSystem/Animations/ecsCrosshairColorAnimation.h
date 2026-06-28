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
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"
#include "ecsCrosshairWidgetAnim.h"
#include "Curves/CurveLinearColor.h"
#include "ecsCrosshairColorAnimation.generated.h"

UCLASS(EditInlineNew, BlueprintType, DefaultToInstanced, DisplayName = "Color Animation")
class EASYCROSSHAIRSYSTEM_API UecsCrosshairColorAnimation : public UecsCrosshairAnimationLayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UCurveLinearColor* AnimationCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	EecsPlayback PlaybackType = EecsPlayback::FORWARD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreA;
	
	virtual void TickLayer(float DeltaTime, SecsCrosshairWidget* Widget) override
	{
		if (TargetLayers.Num() == 0 || !Widget || !AnimationCurve)
			return;

		ElapsedTime += DeltaTime;
		
		float RawProgress = FMath::Clamp(ElapsedTime / AnimationDuration, 0.f, 1.f);
		float Progress = 0.f;
		
		if (PlaybackType == EecsPlayback::FORWARD)
			Progress = RawProgress;
		else if (PlaybackType == EecsPlayback::BACKWARD)
			Progress = 1.f - RawProgress;
		
		
		FLinearColor CurveVal = AnimationCurve->GetLinearColorValue(Progress);

		if (bIgnoreR)
		{
			CurveVal.R = Widget->GetLayerByName(TargetLayers[0]).LayerColor.R;
		}
		if (bIgnoreG)
		{
			CurveVal.G = Widget->GetLayerByName(TargetLayers[0]).LayerColor.G;
		}
		if (bIgnoreB)
		{
			CurveVal.B = Widget->GetLayerByName(TargetLayers[0]).LayerColor.B;
		}
		if (bIgnoreA)
		{
			CurveVal.A = Widget->GetLayerByName(TargetLayers[0]).LayerColor.A;
		}
		
		for (FName LayerName : TargetLayers)
		{
			Widget->SetLayerColorDirect(LayerName, CurveVal);
		}

		if (Progress >= 1.0f)
		{
			if (bLoop && !bStopRequested)
			{
				ElapsedTime = 0.0f;

				if (bResetOnComplete)
				{
					for (FName LayerName : TargetLayers)
					{
						Widget->ResetLayerColor(LayerName);
					}
				}
				// Do not remove from ActiveAnimationLayers, keep animating
			}
			else
			{
				ElapsedTime = 0.0f;
				Widget->ActiveAnimationLayers.Remove(this);

				
				if (bResetOnComplete)
				{
					for (FName LayerName : TargetLayers)
					{
						Widget->ResetLayerColor(LayerName);
					}
				}
			}
		}
	}
};
