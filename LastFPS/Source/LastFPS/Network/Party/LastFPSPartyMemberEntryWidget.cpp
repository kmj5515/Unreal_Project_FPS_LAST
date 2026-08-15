#include "LastFPSPartyMemberEntryWidget.h"
#include "LastFPSPartyMemberData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void ULastFPSPartyMemberEntryWidget::RefreshData(ULastFPSPartyMemberData* MemberData)
{
	if (MemberData)
	{
		if (PlayerNameText)
		{
			PlayerNameText->SetText(FText::FromString(MemberData->PlayerName));
		}

		if (ReadyStatusText)
		{
			FString StatusStr = MemberData->bIsHost ? TEXT("Host") : (MemberData->bIsReady ? TEXT("Ready") : TEXT("Waiting"));
			ReadyStatusText->SetText(FText::FromString(StatusStr));
		}

		if (HostIcon)
		{
			HostIcon->SetVisibility(MemberData->bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}

		BP_OnDataUpdated(MemberData);
	}
}

void ULastFPSPartyMemberEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (ULastFPSPartyMemberData* MemberData = Cast<ULastFPSPartyMemberData>(ListItemObject))
	{
		RefreshData(MemberData);
	}
}
