#include "EditorPlay/LastFPSStartMapPlayToolbar.h"

#include "EditorPlay/LastFPSStartMapPlayService.h"
#include "Framework/Commands/UIAction.h"
#include "Styling/AppStyle.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FLastFPSStartMapPlayToolbar"

void FLastFPSStartMapPlayToolbar::Register()
{
	UToolMenu* PlayToolBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	if (!PlayToolBar)
	{
		return;
	}

	FToolMenuSection& PlaySection = PlayToolBar->FindOrAddSection(TEXT("Play"));
	FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
		TEXT("LastFPSPlayFromStartMap"),
		FUIAction(
			FExecuteAction::CreateStatic(&FLastFPSStartMapPlayService::PlayConfiguredStartMap),
			FCanExecuteAction::CreateStatic(&FLastFPSStartMapPlayService::CanPlayConfiguredStartMap)
		),
		LOCTEXT("PlayFromStartMapLabel", "Start Map"),
		LOCTEXT("PlayFromStartMapTooltip", "Play in Editor with the configured LastFPS start map."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Play"))
	);

	Entry.StyleNameOverride = TEXT("CalloutToolbar");
	PlaySection.AddEntry(Entry);
}

#undef LOCTEXT_NAMESPACE
