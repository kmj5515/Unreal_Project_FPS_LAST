#include "UI/FrontEnd/LastFPSLobbyWidget.h"

#include "Game/LastFPSGameInstance.h"
#include "Game/LastFPSPlayerController.h"
#include "Localization/LastFPSLocalization.h"
#include "UI/Framework/LastFPSButtonBase.h"
#include "UI/Framework/LastFPSContentScreenWidget.h"
#include "UI/Framework/LastFPSScreenRegistry.h"
#include "UI/Framework/LastFPSScreenTypes.h"
#include "UI/Framework/LastFPSUIManagerSubsystem.h"
#include "UI/Framework/LastFPSUITags.h"
#include "UI/Preview/LastFPSPlayerPreviewBuilder.h"
#include "UI/Preview/LastFPSPreviewStageActor.h"
#include "UI/Preview/LastFPSPreviewStageSubsystem.h"

#include "CommonActivatableWidget.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "Groups/CommonButtonGroupBase.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLobbyWidget, Log, All);

void ULastFPSLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TB_PlayerName)
	{
		if (const APlayerState* PS = GetOwningPlayerState())
		{
			TB_PlayerName->SetText(FText::FromString(PS->GetPlayerName()));
		}
	}

	// 매니저가 탭 화면 요청을 이쪽으로 위임할 수 있게 알린다. NPC·퀘스트도 같은 경로를 탄다.
	if (ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this))
	{
		UIManager->RegisterScreenTabHost(this);
	}

	InitializeTabBar();
}

void ULastFPSLobbyWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 아웃게임 화면이 인게임 HUD 를 대신한다. 조준선·체력바가 메뉴 위에 겹쳐 보이지 않게 접는다.
	SetGameLayerHidden(true);

	// 무대는 레벨 시작에 이미 스폰돼 있다. 여기서는 켜기만 한다.
	SetPreviewStageHeld(true);

	// 무대를 볼지 말지는 껍데기의 수명이 정한다. 탭은 어느 카메라를 켤지만 고른다.
	// 여기서 한 번 잡아 두면 시점 지정이 없는 탭으로 가도 배경이 허브 레벨로 튀지 않는다.
	SetStageAsViewTarget(true);

	// 무기 상세 프리뷰가 같은 무대에 무기를 올려 두고 닫히므로, 다시 활성화될 때마다 캐릭터로 되돌린다.
	// SetPreviewStageHeld 는 이미 쥐고 있으면 조기 반환하므로 여기서 따로 부른다.
	RefreshPreviewSubject();
}

void ULastFPSLobbyWidget::NativeOnDeactivated()
{
	// 뷰타깃과 HUD 는 여기서 되돌리지 않는다.
	//
	// 비활성화는 "메뉴가 닫혔다"가 아니라 "위에 무언가 얹혔다"일 수도 있다. 무기 상세 프리뷰가
	// 같은 레이어에 push 되면 껍데기가 먼저 비활성화되는데, 여기서 되돌리면 프리뷰가 잡아 둔
	// 무대 시점을 껍데기가 뒤늦게 게임 화면으로 덮어쓴다.
	// 실제로 화면을 떠나는 시점은 NativeDestruct 이므로 복구는 그쪽이 단독으로 맡는다.

	// 언로드가 아니라 끄기다. 다음에 다시 열 때 스폰·로드 비용을 내지 않는다.
	SetPreviewStageHeld(false);

	Super::NativeOnDeactivated();
}

void ULastFPSLobbyWidget::SetPreviewStageHeld(const bool bHold)
{
	if (bHoldingPreviewStage == bHold)
	{
		return;
	}

	ULastFPSPreviewStageSubsystem* PreviewStage = ULastFPSPreviewStageSubsystem::Get(this);
	if (!PreviewStage)
	{
		return;
	}

	bHoldingPreviewStage = bHold;

	ULastFPSEquipmentSubsystem* Equipment = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULastFPSEquipmentSubsystem>()
		: nullptr;

	if (bHold)
	{
		PreviewStage->AddStageUser();
		RefreshPreviewSubject();

		// 장비를 바꾸면 무대에 선 모습도 따라와야 한다.
		if (Equipment)
		{
			Equipment->OnEquipmentChanged.AddUniqueDynamic(this, &ULastFPSLobbyWidget::HandleEquipmentChanged);
		}
	}
	else
	{
		if (Equipment)
		{
			Equipment->OnEquipmentChanged.RemoveDynamic(this, &ULastFPSLobbyWidget::HandleEquipmentChanged);
		}

		// 대상은 그대로 둔다. 무대가 꺼져 보이지 않으므로 비용은 없고, 다시 열 때 재조립이 필요 없다.
		PreviewStage->RemoveStageUser();
	}
}

void ULastFPSLobbyWidget::RefreshPreviewSubject()
{
	ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this);
	ALastFPSPreviewStageActor* Stage = PreviewStageSubsystem ? PreviewStageSubsystem->GetStage() : nullptr;
	if (!Stage)
	{
		return;
	}

	FLastFPSPreviewSubject Subject;
	if (!LastFPSPlayerPreview::BuildSubject(this, Subject))
	{
		// 폰이 아직 없으면 이전 대상이 남아 엉뚱한 모습이 보인다. 비워 두는 편이 낫다.
		Stage->ClearSubject(LastFPSUITags::PreviewSlot_Character());
		return;
	}

	// 캐릭터 자리에만 올린다. 무기 자리에 올려 둔 것은 그대로 남아 다시 조립할 필요가 없다.
	// 시점은 건드리지 않는다. 어느 시점으로 볼지는 지금 열려 있는 탭이 정한다.
	Stage->SetSubject(LastFPSUITags::PreviewSlot_Character(), Subject);
}

void ULastFPSLobbyWidget::ApplyPreviewViewForScreen(const FGameplayTag& InScreenTag)
{
	ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this);
	const ULastFPSScreenRegistry* Registry = UIManager ? UIManager->GetScreenRegistry() : nullptr;
	const FLastFPSScreenDef* Def = Registry ? Registry->FindScreen(InScreenTag) : nullptr;

	// 시점 지정이 없는 탭이라고 게임 화면으로 되돌리지 않는다. 껍데기가 떠 있는 동안은 계속 무대를 본다.
	// 탭을 오갈 때마다 배경이 허브 레벨과 무대 사이를 오가면 화면이 튄다.
	if (!Def || !Def->PreviewViewTag.IsValid())
	{
		return;
	}

	ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this);
	if (ALastFPSPreviewStageActor* Stage = PreviewStageSubsystem ? PreviewStageSubsystem->GetStage() : nullptr)
	{
		// 어느 카메라를 켤지만 탭이 정한다. 무대를 볼지 말지는 껍데기의 수명이 정한다.
		Stage->SetActiveView(Def->PreviewViewTag);
	}
}

void ULastFPSLobbyWidget::SetGameLayerHidden(const bool bHidden)
{
	if (!bHideGameLayerWhileOpen)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	UPrimaryGameLayout* Layout = PC ? UPrimaryGameLayout::GetPrimaryGameLayout(PC) : nullptr;
	if (!Layout)
	{
		return;
	}

	UCommonActivatableWidgetContainerBase* GameLayer = Layout->GetLayerWidget(LastFPSUITags::Layer_Game());
	if (!GameLayer)
	{
		return;
	}

	if (bHidden)
	{
		// 접기 전 값을 기억해 둔다. 복원할 때 임의의 값을 넣으면 히트테스트 설정이 바뀐다.
		if (!GameLayerPrevVisibility.IsSet())
		{
			GameLayerPrevVisibility = GameLayer->GetVisibility();
		}
		GameLayer->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (GameLayerPrevVisibility.IsSet())
	{
		GameLayer->SetVisibility(GameLayerPrevVisibility.GetValue());
		GameLayerPrevVisibility.Reset();
	}
}

void ULastFPSLobbyWidget::SetStageAsViewTarget(const bool bUseStage)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (!bUseStage)
	{
		// 들어올 때 기억해 둔 원래 뷰타깃으로만 되돌린다. 무대를 쓴 적이 없으면 할 일이 없다.
		if (AActor* Previous = PreviewPrevViewTarget.Get())
		{
			PC->SetViewTargetWithBlend(Previous, PreviewViewBlendTime);
		}
		PreviewPrevViewTarget = nullptr;
		return;
	}

	ULastFPSPreviewStageSubsystem* PreviewStageSubsystem = ULastFPSPreviewStageSubsystem::Get(this);
	ALastFPSPreviewStageActor* Stage = PreviewStageSubsystem ? PreviewStageSubsystem->GetStage() : nullptr;
	if (!Stage || PC->GetViewTarget() == Stage)
	{
		return;
	}

	// 무대로 넘어가기 전 화면을 기억해 둔다. 메뉴를 닫으면 여기로 돌아온다.
	PreviewPrevViewTarget = PC->GetViewTarget();
	PC->SetViewTargetWithBlend(Stage, PreviewViewBlendTime);
}

void ULastFPSLobbyWidget::HandleEquipmentChanged(
	ELastFPSEquipmentSlotType /*SlotType*/, int32 /*SlotIndex*/)
{
	// 어느 슬롯이 바뀌었는지는 보지 않는다. 무대에 세우는 조립 규칙은 빌더 한 곳만 안다.
	RefreshPreviewSubject();
}

void ULastFPSLobbyWidget::NativeDestruct()
{
	// 비활성화 없이 곧바로 파괴되는 경로가 있어 여기서도 반납한다.
	// 뷰타깃이 무대에 남으면 게임 화면으로 못 돌아오고, 참조 수가 새면 무대가 계속 켜져 있는다.
	SetStageAsViewTarget(false);
	SetGameLayerHidden(false);
	SetPreviewStageHeld(false);

	if (ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this))
	{
		UIManager->UnregisterScreenTabHost(this);
	}

	if (TabGroup)
	{
		TabGroup->OnSelectedButtonBaseChanged.RemoveDynamic(this, &ULastFPSLobbyWidget::HandleTabSelected);
		TabGroup->RemoveAll();
		TabGroup = nullptr;
	}

	TabContents.Reset();

	Super::NativeDestruct();
}

void ULastFPSLobbyWidget::InitializeTabBar()
{
	if (!HBox_Tabs || !Switcher_Content)
	{
		UE_LOG(LogLastFPSLobbyWidget, Warning,
			TEXT("%s: 탭바를 구성할 수 없습니다(HBox_Tabs=%s, Switcher_Content=%s)."),
			*GetName(),
			HBox_Tabs ? TEXT("OK") : TEXT("없음"),
			Switcher_Content ? TEXT("OK") : TEXT("없음"));
		return;
	}

	// 패널에 실제로 배치된 버튼만 탭으로 삼는다. 버튼 이름을 코드가 알 필요가 없다.
	TArray<UCommonButtonBase*> TabButtons;
	const int32 ChildCount = HBox_Tabs->GetChildrenCount();
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		if (UCommonButtonBase* TabButton = Cast<UCommonButtonBase>(HBox_Tabs->GetChildAt(ChildIndex)))
		{
			TabButtons.Add(TabButton);
		}
	}

	if (TabButtons.Num() == 0)
	{
		return;
	}

	// 탭 목록의 단일 출처는 레지스트리다. 여기서는 순서만 버튼 배치와 맞춘다.
	if (ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this))
	{
		if (const ULastFPSScreenRegistry* Registry = UIManager->GetScreenRegistry())
		{
			Registry->GetTabScreensForHost(ScreenTag, TabScreenTags);
		}
	}

	if (TabScreenTags.Num() != TabButtons.Num())
	{
		// 개수가 어긋나면 남는 쪽은 선택 표시만 되고 화면이 바뀌지 않는다. 저작 실수를 조용히 넘기지 않는다.
		UE_LOG(LogLastFPSLobbyWidget, Warning,
			TEXT("%s: 탭 버튼 %d개와 '%s' 를 호스트로 지정한 탭 화면 %d개가 일치하지 않습니다. "
				 "ScreenRegistry 의 HostScreenTag/TabOrder 를 확인하세요."),
			*GetName(), TabButtons.Num(), *ScreenTag.ToString(), TabScreenTags.Num());
	}

	TabGroup = NewObject<UCommonButtonGroupBase>(this);
	// 탭은 항상 하나가 선택된 상태여야 현재 위치를 알 수 있다.
	TabGroup->SetSelectionRequired(true);

	for (UCommonButtonBase* TabButton : TabButtons)
	{
		// 그룹은 '선택 가능한' 버튼만 상태로 관리한다. WBP 저작값에 기대지 않고 여기서 보장한다.
		TabButton->SetIsSelectable(true);
		TabGroup->AddWidget(TabButton);
	}

	TabGroup->OnSelectedButtonBaseChanged.AddDynamic(this, &ULastFPSLobbyWidget::HandleTabSelected);

	// 외부에서 특정 탭을 지목해 들어왔으면(예: I 키로 인벤토리) 그 탭을 첫 탭으로 삼는다.
	// 기본 탭을 먼저 만들면 쓰지도 않을 화면 위젯과 그 참조 에셋을 통째로 로드하게 된다.
	int32 InitialIndex = TabButtons.IsValidIndex(DefaultTabIndex) ? DefaultTabIndex : 0;
	if (ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this))
	{
		const FGameplayTag RequestedTabTag = UIManager->ConsumePendingTabScreenTag();
		const int32 RequestedIndex = TabScreenTags.IndexOfByKey(RequestedTabTag);
		if (RequestedIndex != INDEX_NONE && TabButtons.IsValidIndex(RequestedIndex))
		{
			InitialIndex = RequestedIndex;
		}
		else if (RequestedTabTag.IsValid())
		{
			UE_LOG(LogLastFPSLobbyWidget, Warning,
				TEXT("%s: 요청된 탭 '%s' 을(를) 찾지 못해 기본 탭을 엽니다."),
				*GetName(), *RequestedTabTag.ToString());
		}
	}

	// SelectButtonAtIndex 가 변경 통보를 그대로 쏘기 때문에, 선택과 화면 표시를 여기서 한 번만 처리한다.
	bApplyingInitialTab = true;
	TabGroup->SelectButtonAtIndex(InitialIndex);
	bApplyingInitialTab = false;

	if (TabScreenTags.IsValidIndex(InitialIndex))
	{
		ShowScreenTab(TabScreenTags[InitialIndex]);
	}
}

bool ULastFPSLobbyWidget::HostsScreenTab(const FGameplayTag& InScreenTag) const
{
	return InScreenTag.IsValid() && TabScreenTags.Contains(InScreenTag);
}

UCommonActivatableWidget* ULastFPSLobbyWidget::ShowScreenTab(const FGameplayTag& InScreenTag)
{
	if (!Switcher_Content)
	{
		return nullptr;
	}

	UCommonActivatableWidget* Content = FindOrCreateTabContent(InScreenTag);
	if (!Content)
	{
		return nullptr;
	}

	Switcher_Content->SetActiveWidget(Content);

	// 화면마다 뒤에 보여줄 것이 다르다. 어떤 시점인지는 화면 정의가 알고 껍데기는 전달만 한다.
	ApplyPreviewViewForScreen(InScreenTag);

	// 외부(NPC·퀘스트)에서 들어온 요청이면 탭 버튼도 그 위치로 맞춰 준다.
	const int32 TabIndex = TabScreenTags.IndexOfByKey(InScreenTag);
	if (TabGroup && TabIndex != INDEX_NONE && TabGroup->GetSelectedButtonIndex() != TabIndex)
	{
		bApplyingInitialTab = true;
		TabGroup->SelectButtonAtIndex(TabIndex);
		bApplyingInitialTab = false;
	}

	return Content;
}

UCommonActivatableWidget* ULastFPSLobbyWidget::FindOrCreateTabContent(const FGameplayTag& InScreenTag)
{
	if (const TObjectPtr<UCommonActivatableWidget>* Existing = TabContents.Find(InScreenTag))
	{
		if (*Existing)
		{
			return *Existing;
		}
		TabContents.Remove(InScreenTag);
	}

	ULastFPSUIManagerSubsystem* UIManager = ULastFPSUIManagerSubsystem::Get(this);
	const ULastFPSScreenRegistry* Registry = UIManager ? UIManager->GetScreenRegistry() : nullptr;
	const FLastFPSScreenDef* Def = Registry ? Registry->FindScreen(InScreenTag) : nullptr;
	if (!Def)
	{
		UE_LOG(LogLastFPSLobbyWidget, Warning,
			TEXT("%s: ScreenRegistry 에 '%s' 정의가 없어 탭을 만들 수 없습니다."),
			*GetName(), *InScreenTag.ToString());

		// 빈 화면만 남기지 않도록 기존 "준비 중" 안내를 유지한다.
		if (ALastFPSPlayerController* PC = GetOwningPlayer<ALastFPSPlayerController>())
		{
			const FText FeatureName = FText::FromName(InScreenTag.GetTagName());
			PC->ShowNotice(
				FeatureName,
				FText::Format(
					FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::LobbyUnavailableFeature),
					FeatureName));
		}
		return nullptr;
	}

	// 탭에 처음 들어갈 때만 로드한다. 화면 전부를 미리 들고 있을 이유가 없다.
	TSubclassOf<UCommonActivatableWidget> WidgetClass = Def->WidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogLastFPSLobbyWidget, Error,
			TEXT("%s: '%s' 의 WidgetClass 로드에 실패했습니다."), *GetName(), *InScreenTag.ToString());
		return nullptr;
	}

	UCommonActivatableWidget* Content = CreateWidget<UCommonActivatableWidget>(GetOwningPlayer(), WidgetClass);
	if (!Content)
	{
		return nullptr;
	}

	// 상단바는 껍데기가 소유한다. 콘텐츠가 자기 타이틀바를 또 그리면 두 겹으로 보인다.
	if (ULastFPSContentScreenWidget* ContentScreen = Cast<ULastFPSContentScreenWidget>(Content))
	{
		ContentScreen->SetHostedInShell(true);
	}

	Switcher_Content->AddChild(Content);
	TabContents.Add(InScreenTag, Content);
	return Content;
}

void ULastFPSLobbyWidget::HandleTabSelected(UCommonButtonBase* /*AssociatedButton*/, const int32 ButtonIndex)
{
	if (bApplyingInitialTab || !TabScreenTags.IsValidIndex(ButtonIndex))
	{
		return;
	}

	ShowScreenTab(TabScreenTags[ButtonIndex]);
}

void ULastFPSLobbyWidget::HandleBackToMainClicked()
{
	if (ULastFPSGameInstance* GI = GetGameInstance<ULastFPSGameInstance>())
	{
		GI->RequestTravelToMainMenu();
	}
}
