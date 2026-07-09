#include "EditorPlay/LastFPSStartMapPlayToolbar.h"

#include "EditorPlay/LastFPSStartMapPlayService.h"
#include "Framework/Commands/UIAction.h"
#include "Styling/AppStyle.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FLastFPSStartMapPlayToolbar"

void FLastFPSStartMapPlayToolbar::Register()
{
	// 플레이 버튼
	UToolMenu* PlayToolBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	if (!PlayToolBar)
	{
		return;
	}

	FToolMenuSection& PlaySection = PlayToolBar->FindOrAddSection(TEXT("Play"));
	FToolMenuEntry Entry = FToolMenuEntry::InitWidget(
		TEXT("LastFPSPlayFromStartMap"),
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(LOCTEXT("PlayFromStartMapTooltip", "Play in Editor with the configured LastFPS start map."))
		.OnClicked_Lambda([]()
		{
			FLastFPSStartMapPlayService::PlayConfiguredStartMap();
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Play"))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.0f, 0.1f)))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PlayFromStartMapLabel", "게임 플레이"))
			]
		],
		FText::GetEmpty()
	);

	Entry.StyleNameOverride = TEXT("CalloutToolbar");
	PlaySection.AddEntry(Entry);
}

#undef LOCTEXT_NAMESPACE
