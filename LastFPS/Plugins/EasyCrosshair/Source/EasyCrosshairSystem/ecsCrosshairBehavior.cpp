// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com


#include "ecsCrosshairBehavior.h"
#include "ecsCrosshairWidget.h"

void UecsCrosshairBehavior::Tick()
{
	ReceiveTick();
}


void UecsCrosshairBehavior::BeginBehavior()
{
	ReceiveBeginBehavior();
}

void UecsCrosshairBehavior::SetLayerTransform(FName LayerName, FVector2D Transform, float Rotation, float Scale)
{
    CrosshairWidget->CrosshairWidget->SetLayerTransform(LayerName, Transform, Rotation, Scale);
}

void UecsCrosshairBehavior::SetLayerColor(FName LayerName, const FLinearColor& InColor)
{
	CrosshairWidget->CrosshairWidget->SetLayerColorDirect(LayerName, InColor);
}

void UecsCrosshairBehavior::PlayAnimation(FName AnimationType)
{
	FecsCrosshairAnimation Animation = CrosshairWidget->GetCrosshairEditorAsset()->GetAnimationByName(AnimationType);
	CrosshairWidget->CrosshairWidget->PlayAnimation(Animation); 
}

APawn* UecsCrosshairBehavior::GetOwningPawn()
{
	return CrosshairWidget->GetOwningPlayer()->GetPawn();
}

FHitResult UecsCrosshairBehavior::LineTraceByChannel(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel)
{
    FHitResult HitResult;
    APawn* Pawn = GetOwningPawn();
    if (!Pawn)
    {
        return HitResult;
    }

    UWorld* World = Pawn->GetWorld();
    if (!World)
    {
        return HitResult;
    }

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Pawn);

    World->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, Params);
    return HitResult;
}

void UecsCrosshairBehavior::ResetLayerColor(FName LayerName)
{
    if (!CrosshairWidget)
    {
        return;
    }

    // Get the default color from the asset's layer data
    CrosshairWidget->CrosshairWidget->ResetLayerColor(LayerName);
}