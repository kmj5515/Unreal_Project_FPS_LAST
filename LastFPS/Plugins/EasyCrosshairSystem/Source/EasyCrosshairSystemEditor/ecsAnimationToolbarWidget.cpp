// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsAnimationToolbarWidget.h"

#include "Editor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Engine/StaticMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SSplitter.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"

#define LOCTEXT_NAMESPACE "DCrosshairEditor"

void SecsAnimationToolbarWidget::Construct(const FArguments& InArgs)
{
	CrosshairWidget = InArgs._CrosshairWidget;

	TitleText = SNew(STextBlock)
		.Text(LOCTEXT("AnimationToolbarTitle", "Animations"))
		.Font(FAppStyle::GetFontStyle("NormalFont"));
	
	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
			.ContentPadding(FMargin(4.0f, 2.0f))
			.HasDownArrow(true)
			.ButtonContent()
			[
				TitleText.ToSharedRef()
			]
			.OnGetMenuContent(this, &SecsAnimationToolbarWidget::GenerateMenuContent)
		]
	];
}

TSharedRef<SWidget> SecsAnimationToolbarWidget::GenerateMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		FText::FromString("Default"),
		FText::FromString(EditingAsset->Description.ToString()),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SecsAnimationToolbarWidget::OnDefaultAnimationSelected)));
	
	if (!EditingAsset || EditingAsset->Animations.Num() == 0)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString("No Animations Found"),
			FText::FromString("There are no animations to display."),
			FSlateIcon(),
			FUIAction());
	}
	else
	{
		for (const auto& CrosshairAnimation : EditingAsset->Animations)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString(CrosshairAnimation.Name.ToString()),
				FText::FromString(EditingAsset->Description.ToString()),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SecsAnimationToolbarWidget::PlayAnimationClicked, CrosshairAnimation)));
		}
	}

	return MenuBuilder.MakeWidget();
}


void SecsAnimationToolbarWidget::OnDefaultAnimationSelected()
{
	CurrentAnimationName = "Default";
	TitleText->SetText(FText::FromString(CurrentAnimationName.ToString()));
	CrosshairWidget->StopAllAnimations(false);
}

void SecsAnimationToolbarWidget::PlayAnimationClicked(FecsCrosshairAnimation CrosshairAnimation)
{
	CurrentAnimationName = CrosshairAnimation.Name;
	TitleText->SetText(FText::FromString(CurrentAnimationName.ToString()));
	CrosshairWidget->PlayAnimation(CrosshairAnimation);
}

#undef LOCTEXT_NAMESPACE 