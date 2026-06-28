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
#include "GameFramework/Pawn.h"
#include "UObject/Object.h"
#include "ecsCrosshairBehavior.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, Blueprintable, BlueprintType, DefaultToInstanced)
class EASYCROSSHAIRSYSTEM_API UecsCrosshairBehavior : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "Tick"))
	void ReceiveTick();
	virtual void Tick();

	// Begin Behavior
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "BeginBehavior"))
	void ReceiveBeginBehavior();
	virtual void BeginBehavior();


	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	void SetLayerTransform(FName LayerName, FVector2D Transform, float Rotation, float Scale);
	
	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	void SetLayerColor(FName LayerName, const FLinearColor& InColor);

	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	void PlayAnimation(FName AnimationType);

	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	APawn* GetOwningPawn();

	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	FHitResult LineTraceByChannel(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel = ECC_Visibility);

	UFUNCTION(BlueprintCallable, Category= "EasyCrosshair")
	void ResetLayerColor(FName LayerName);
	
	UPROPERTY()
	UecsCrosshairWidget* CrosshairWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default")
	float TickRate;
};
