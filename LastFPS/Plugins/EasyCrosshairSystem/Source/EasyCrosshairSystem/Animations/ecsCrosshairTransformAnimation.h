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
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "ecsCrosshairTransformAnimation.generated.h"

UCLASS(EditInlineNew, BlueprintType, DefaultToInstanced, DisplayName = "Transform Animation")
class EASYCROSSHAIRSYSTEM_API UecsCrosshairTransformAnimation : public UecsCrosshairAnimationLayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UCurveVector* AnimationCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UCurveFloat* ScaleCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bMirrorCurveValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIgnoreScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	EecsPlayback PlaybackType = EecsPlayback::FORWARD;

	virtual void TickLayer(float DeltaTime, SecsCrosshairWidget* Widget) override
	{
		if (TargetLayers.Num() == 0 || !Widget)
			return;

		ElapsedTime += DeltaTime;

		float RawProgress = FMath::Clamp(ElapsedTime / AnimationDuration, 0.f, 1.f);
		float Progress = RawProgress;

		// Handle playback direction
		if (PlaybackType == EecsPlayback::BACKWARD)
		{
			Progress = 1.0f - RawProgress;
		}
		
		for (FName LayerName : TargetLayers)
		{
			FVector2D ResultOffset = Widget->GetLayerByName(LayerName).Transform;
			float ResultRotation = Widget->GetLayerByName(LayerName).Rotation;
			float ResultScale = Widget->GetLayerByName(LayerName).Scale;

			if (AnimationCurve)
			{
				FVector VectorCurveValue = AnimationCurve->GetVectorValue(Progress);
				if (bMirrorCurveValue)
				{
					VectorCurveValue.X = -VectorCurveValue.X;
					VectorCurveValue.Y = -VectorCurveValue.Y;
					VectorCurveValue.Z = -VectorCurveValue.Z;
				}

				if (bIgnoreX)
					ResultOffset.X = Widget->GetLayerByName(LayerName).Transform.X;
				else
					ResultOffset.X = VectorCurveValue.X;

				if (bIgnoreY)
					ResultOffset.Y = Widget->GetLayerByName(LayerName).Transform.Y;
				else
					ResultOffset.Y = VectorCurveValue.Y;

				if (bIgnoreZ)
					ResultRotation = Widget->GetLayerByName(LayerName).Rotation;
				else
					ResultRotation = VectorCurveValue.Z;
			}

			if (ScaleCurve)
			{
				float FloatCurveValue = ScaleCurve->GetFloatValue(Progress);

				if (bIgnoreScale)
					ResultScale = Widget->GetLayerByName(LayerName).Scale;
				else
					ResultScale = FloatCurveValue;
			}

			Widget->SetLayerTransform(LayerName, ResultOffset, ResultRotation, ResultScale);
		}

		bool bAnimationDone = (PlaybackType == EecsPlayback::FORWARD) ? (RawProgress >= 1.0f) : (RawProgress >= 1.0f);

		if (bAnimationDone)
		{
			if (bLoop && !bStopRequested)
			{
				ElapsedTime = 0.0f;

				if (bResetOnComplete)
				{
					for (FName LayerName : TargetLayers)
					{
						Widget->SetLayerTransform(LayerName,
							FVector2D(Widget->GetLayerByName(LayerName).Transform.X,
								Widget->GetLayerByName(LayerName).Transform.Y),
							Widget->GetLayerByName(LayerName).Rotation,
							Widget->GetLayerByName(LayerName).Scale);
					}
				}
			}
			else
			{
				ElapsedTime = 0.0f;
				Widget->ActiveAnimationLayers.Remove(this);

				if (bResetOnComplete)
				{
					for (FName LayerName : TargetLayers)
					{
						Widget->SetLayerTransform(LayerName,
							FVector2D(Widget->GetLayerByName(LayerName).Transform.X,
								Widget->GetLayerByName(LayerName).Transform.Y),
							Widget->GetLayerByName(LayerName).Rotation,
							Widget->GetLayerByName(LayerName).Scale);
					}
				}
			}
		}
	}
};