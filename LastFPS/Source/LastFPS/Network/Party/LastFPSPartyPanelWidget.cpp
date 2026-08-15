#include "LastFPSPartyPanelWidget.h"
#include "Components/ListView.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LastFPSPartyGameState.h"
#include "LastFPSPartyPlayerState.h"
#include "LastFPSPartyPlayerController.h"
#include "LastFPSPartyMemberData.h"
#include "LastFPSPartyMemberEntryWidget.h"
#include "UI/Common/LastFPSNoticeWidget.h"

ULastFPSPartyPanelWidget::ULastFPSPartyPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULastFPSPartyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &ULastFPSPartyPanelWidget::OnApplyButtonClicked);
	}

	GetWorld()->GetTimerManager().SetTimer(PollingTimerHandle, this, &ULastFPSPartyPanelWidget::TryBindToGameState, 0.5f, true);
}

void ULastFPSPartyPanelWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(PollingTimerHandle);
	
	if (CachedGameState.IsValid())
	{
		CachedGameState->OnPartyMembersUpdated.RemoveDynamic(this, &ULastFPSPartyPanelWidget::RefreshPartyList);
	}
	
	Super::NativeDestruct();
}

void ULastFPSPartyPanelWidget::TryBindToGameState()
{
	if (!CachedGameState.IsValid())
	{
		if (ALastFPSPartyGameState* GS = GetWorld()->GetGameState<ALastFPSPartyGameState>())
		{
			CachedGameState = GS;
			GS->OnPartyMembersUpdated.AddDynamic(this, &ULastFPSPartyPanelWidget::RefreshPartyList);
		}
	}

	if (CachedGameState.IsValid())
	{
		RefreshPartyList();
	}
}

void ULastFPSPartyPanelWidget::OnApplyButtonClicked()
{
	if (ALastFPSPartyPlayerController* PC = Cast<ALastFPSPartyPlayerController>(GetOwningPlayer()))
	{
		if (ALastFPSPartyPlayerState* PS = PC->GetPlayerState<ALastFPSPartyPlayerState>())
		{
			if (PS->IsPartyHost())
			{
				bool bAllReady = true;
				if (CachedGameState.IsValid())
				{
					for (ALastFPSPartyPlayerState* MemberPS : CachedGameState->GetPartyMembers())
					{
						if (!MemberPS->IsPartyReady() && !MemberPS->IsPartyHost())
						{
							bAllReady = false;
							break;
						}
					}
				}

				if (bAllReady)
				{
					PC->ServerRequestStartPartyGame();
				}
				else
				{
					FLastFPSNoticeParams Params;
					Params.Title = FText::FromString(TEXT("알림"));
					Params.Body = FText::FromString(TEXT("준비하지 않은 플레이어가 있습니다."));
					ULastFPSNoticeWidget::ShowPopup(this, Params);
				}
			}
			else
			{
				PC->ServerSetPartyReady(!PS->IsPartyReady());
			}
		}
	}
}

void ULastFPSPartyPanelWidget::RefreshPartyList()
{
	if (!PartyMemberList || !CachedGameState.IsValid()) return;

	TArray<ALastFPSPartyPlayerState*> Members = CachedGameState->GetPartyMembers();
	
	if (PlayerCountText)
	{
		FString CountStr = FString::Printf(TEXT("인원 (%d/4)"), Members.Num());
		PlayerCountText->SetText(FText::FromString(CountStr));
	}

	if (ALastFPSPartyPlayerController* PC = Cast<ALastFPSPartyPlayerController>(GetOwningPlayer()))
	{
		if (ALastFPSPartyPlayerState* LocalPS = PC->GetPlayerState<ALastFPSPartyPlayerState>())
		{
			if (ApplyText)
			{
				if (LocalPS->IsPartyHost())
				{
					ApplyText->SetText(FText::FromString(TEXT("게임 시작")));
				}
				else
				{
					if (LocalPS->IsPartyReady())
					{
						ApplyText->SetText(FText::FromString(TEXT("준비 취소")));
					}
					else
					{
						ApplyText->SetText(FText::FromString(TEXT("준비")));
					}
				}
			}
		}
	}

	TArray<UObject*> CurrentItems = PartyMemberList->GetListItems();

	for (int32 i = CurrentItems.Num() - 1; i >= 0; --i)
	{
		ULastFPSPartyMemberData* Data = Cast<ULastFPSPartyMemberData>(CurrentItems[i]);
		if (Data && !Members.Contains(Data->PlayerState))
		{
			PartyMemberList->RemoveItem(Data);
		}
	}

	CurrentItems = PartyMemberList->GetListItems();

	for (ALastFPSPartyPlayerState* PS : Members)
	{
		if (!PS) continue;

		ULastFPSPartyMemberData* FoundData = nullptr;
		for (UObject* Item : CurrentItems)
		{
			ULastFPSPartyMemberData* Data = Cast<ULastFPSPartyMemberData>(Item);
			if (Data && Data->PlayerState == PS)
			{
				FoundData = Data;
				break;
			}
		}

		if (FoundData)
		{
			FoundData->PlayerName = PS->GetPlayerName();
			FoundData->bIsReady = PS->IsPartyReady();
			FoundData->bIsHost = PS->IsPartyHost();
			
			if (UUserWidget* EntryWidget = PartyMemberList->GetEntryWidgetFromItem(FoundData))
			{
				if (ULastFPSPartyMemberEntryWidget* MemberWidget = Cast<ULastFPSPartyMemberEntryWidget>(EntryWidget))
				{
					MemberWidget->RefreshData(FoundData);
				}
			}
		}
		else
		{
			ULastFPSPartyMemberData* NewData = NewObject<ULastFPSPartyMemberData>(this);
			NewData->PlayerState = PS;
			NewData->PlayerName = PS->GetPlayerName();
			NewData->bIsReady = PS->IsPartyReady();
			NewData->bIsHost = PS->IsPartyHost();
			PartyMemberList->AddItem(NewData);
			
			PS->OnPartyStateChanged.RemoveDynamic(this, &ULastFPSPartyPanelWidget::OnPartyStateChanged);
			PS->OnPartyStateChanged.AddDynamic(this, &ULastFPSPartyPanelWidget::OnPartyStateChanged);
		}
	}
}

void ULastFPSPartyPanelWidget::OnPartyStateChanged(ALastFPSPartyPlayerState* PS)
{
	RefreshPartyList();
}
