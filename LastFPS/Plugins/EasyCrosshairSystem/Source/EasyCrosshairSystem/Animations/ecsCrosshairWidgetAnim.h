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
#include "Curves/CurveVector.h" 
#include "ecsCrosshairWidgetAnim.generated.h"

UENUM(BlueprintType)
enum class EecsPlayback : uint8
{
	FORWARD		= 0 UMETA(DisplayName = "Forward", ToolTip = "Play animation forward"),
	BACKWARD	= 1 UMETA(DisplayName = "Backward", ToolTip = "Play animation backward"),
};


UENUM(BlueprintType)
enum class EecsCrosshairAnimationOption : uint8
{
	Linear = 0 UMETA(Grouping = Linear, DisplayName = "Linear", ToolTip = "Linear interpolation"),
	Cubic UMETA(Grouping = Cubic, DisplayName = "Cubic In", ToolTip = "Cubic-in interpolation"),
	HermiteCubic UMETA(Grouping = Cubic, DisplayName = "Hermite-Cubic InOut", ToolTip = "Hermite-Cubic"),
	Sinusoidal UMETA(Grouping = Sinusoidal, DisplayName = "Sinusoidal", ToolTip = "Sinusoidal interpolation"),
	QuadraticInOut UMETA(Grouping = Quadratic, DisplayName = "Quadratic InOut", ToolTip = "Quadratic in-out interpolation"),
	CubicInOut UMETA(Grouping = Cubic, DisplayName = "Cubic InOut", ToolTip = "Cubic in-out interpolation"),
	QuarticInOut UMETA(Grouping = Quartic, DisplayName = "Quartic InOut", ToolTip = "Quartic in-out interpolation"),
	QuinticInOut UMETA(Grouping = Quintic, DisplayName = "Quintic InOut", ToolTip = "Quintic in-out interpolation"),
	CircularIn UMETA(Grouping = Circular, DisplayName = "Circular In", ToolTip = "Circular-in interpolation"),
	CircularOut UMETA(Grouping = Circular, DisplayName = "Circular Out", ToolTip = "Circular-out interpolation"),
	CircularInOut UMETA(Grouping = Circular, DisplayName = "Circular InOut", ToolTip = "Circular in-out interpolation"),
	ExpIn UMETA(Grouping = Exponential, DisplayName = "Exponential In", ToolTip = "Exponential-in interpolation"),
	ExpOut UMETA(Grouping = Exponential, DisplayName = "Exponential Out", ToolTip = "Exponential-Out interpolation"),
	ExpInOut UMETA(Grouping = Exponential, DisplayName = "Exponential InOut", ToolTip = "Exponential in-out interpolation"),
	Custom UMETA(Grouping = Custom, DisplayName = "Custom", ToolTip = "Custom interpolation, will use custom curve inside an FAlphaBlend or linear if none has been set"),
};

UCLASS(Abstract, EditInlineNew, BlueprintType, DefaultToInstanced)
class EASYCROSSHAIRSYSTEM_API UecsCrosshairAnimationLayer : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<FName> TargetLayers;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bLoop = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	bool bResetOnComplete = true;
	
	virtual void TickLayer(float DeltaTime, class SecsCrosshairWidget* Widget) {}

	bool bIsDone = false;
	bool bStopRequested = false;
	float ElapsedTime = 0.0f;
	float PreviousCurveValue = 0.0f; // Add this as a member variable
	bool bIsDirty = false;
	float AnimationDuration = 1.0f;
};





USTRUCT(BlueprintType)
struct EASYCROSSHAIRSYSTEM_API FecsCrosshairAnimation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Animation")
	FName Name;

	// Editable inline instances of animation layers
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Crosshair Animation")
	TArray<UecsCrosshairAnimationLayer*> AnimationLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair Animation")
	float AnimationDuration = 1.0f;
};
