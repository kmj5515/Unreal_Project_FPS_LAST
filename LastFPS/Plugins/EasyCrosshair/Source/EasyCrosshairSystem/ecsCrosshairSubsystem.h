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
#include "ecsCrosshairWidget.h"
#include "Subsystems/WorldSubsystem.h"
#include "ecsCrosshairSubsystem.generated.h"

/**
 * 
 */
UCLASS(DisplayName="Easy Crosshair Subsystem") 
class EASYCROSSHAIRSYSTEM_API UecsCrosshairSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="EasyCrosshair") void SetupCrosshair(UecsCrosshairEditorAsset* DynamicCrosshair);
	UFUNCTION(BlueprintCallable, Category="EasyCrosshair") void RemoveCrosshair();
	UFUNCTION(BlueprintCallable, Category="EasyCrosshair") void RunAnimation(FName AnimationName, float AnimationDuration = 0.0f);
	UFUNCTION(BlueprintCallable, Category="EasyCrosshair") void StopAnimation(FName AnimationName);
	UFUNCTION(BlueprintCallable, Category="EasyCrosshair") UecsCrosshairWidget* GetCrosshairWidget();
private:
	UPROPERTY()
	TObjectPtr<UecsCrosshairWidget> CrosshairWidget;
};
