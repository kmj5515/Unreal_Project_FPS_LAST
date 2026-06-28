// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#pragma once

#include "ecsEditorStyle.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

TSharedPtr<FSlateStyleSet> FEasyCrosshairSystemEditorStyle::StyleInstance = nullptr;

void FEasyCrosshairSystemEditorStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	// Create a new style set
	StyleInstance = MakeShareable(new FSlateStyleSet("EasyCrosshairSystemEditorStyle"));

	// Set the content root to the Resources folder in your plugin
	FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("EasyCrosshairSystem"))->GetBaseDir() / TEXT("Resources");
	StyleInstance->SetContentRoot(ContentDir);

	// Register your custom icon as an image brush
	// Example: 16x16 icon
	StyleInstance->Set("EasyCrosshairSystemEditor.Icon", new FSlateImageBrush(
		StyleInstance->RootToContentDir(TEXT("EasyCrosshairSystemLogo.png")),
		FVector2D(256.0f, 256.0f)
	));

	// Finally, register the style with the Slate system
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FEasyCrosshairSystemEditorStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

const ISlateStyle& FEasyCrosshairSystemEditorStyle::Get()
{
	return *StyleInstance;
}

FName FEasyCrosshairSystemEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("EasyCrosshairSystemEditorStyle"));
	return StyleSetName;
}
