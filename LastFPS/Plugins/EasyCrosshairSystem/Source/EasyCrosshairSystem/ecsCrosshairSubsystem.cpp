// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com


#include "ecsCrosshairSubsystem.h"

void UecsCrosshairSubsystem::SetupCrosshair(UecsCrosshairEditorAsset* DynamicCrosshair)
{
	if (CrosshairWidget)
		return;
	
	CrosshairWidget = CreateWidget<UecsCrosshairWidget>(GetWorld(), UecsCrosshairWidget::StaticClass());
	if (CrosshairWidget)
	{
		CrosshairWidget->ConfigureCrosshairWidget(DynamicCrosshair);
		CrosshairWidget->AddToViewport();
	}
}

void UecsCrosshairSubsystem::RemoveCrosshair()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->RemoveFromParent();
		CrosshairWidget = nullptr;
	}
}

void UecsCrosshairSubsystem::RunAnimation(FName AnimationType, float AnimationDuration)
{
	FecsCrosshairAnimation Animation = CrosshairWidget->GetCrosshairEditorAsset()->GetAnimationByName(AnimationType);

	if (AnimationDuration != 0.f)
		Animation.AnimationDuration = AnimationDuration; // Override duration if specified
	
	for (auto Element : Animation.AnimationLayers)
		Element->bStopRequested = false; // Reset stop request
	
	CrosshairWidget->CrosshairWidget->PlayAnimation(Animation);
}

void UecsCrosshairSubsystem::StopAnimation(FName AnimationName)
{
	FecsCrosshairAnimation Animation = CrosshairWidget->GetCrosshairEditorAsset()->GetAnimationByName(AnimationName);
	CrosshairWidget->CrosshairWidget->StopAnimation(Animation);
}

UecsCrosshairWidget* UecsCrosshairSubsystem::GetCrosshairWidget()
{
	return CrosshairWidget;
}
