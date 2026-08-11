#include "UI/Map/LastFPSMapScreenWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Game/LastFPSPlayerController.h"
#include "InputCoreTypes.h"
#include "Game/Travel/LastFPSLevelTravelSubsystem.h"
#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "UI/Framework/LastFPSTravelEntryButton.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSMapScreen, Log, All);

void ULastFPSMapScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWidget* Target = GetTransformTarget())
	{
		// 피벗을 좌상단으로 옮겨 이동량과 배율을 서로 얽히지 않게 다룬다.
		// 기본값(0.5, 0.5)이면 확대할 때마다 중심이 밀려 계산이 꼬인다.
		Target->SetRenderTransformPivot(FVector2D::ZeroVector);
		ApplyMapTransform();
	}

	if (!Box_Destinations)
	{
		UE_LOG(LogLastFPSMapScreen, Warning,
			TEXT("%s: Box_Destinations 가 없어 목적지를 연결하지 못했습니다."), *GetName());
		return;
	}

	const int32 ChildCount = Box_Destinations->GetChildrenCount();
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		ULastFPSTravelEntryButton* Destination =
			Cast<ULastFPSTravelEntryButton>(Box_Destinations->GetChildAt(ChildIndex));
		if (!Destination)
		{
			continue;
		}

		// 어느 버튼이 눌렸는지 알아야 하므로 버튼 자신을 페이로드로 함께 넘긴다.
		Destination->OnClicked().AddUObject(
			this,
			&ULastFPSMapScreenWidget::HandleDestinationClicked,
			TWeakObjectPtr<ULastFPSTravelEntryButton>(Destination));

		BoundDestinations.Add(Destination);
	}

	if (ULastFPSQuestSubsystem* QuestSubsystem = ULastFPSQuestSubsystem::Get(this))
	{
		QuestSubsystem->OnQuestStateChanged.AddUniqueDynamic(
			this, &ULastFPSMapScreenWidget::RefreshDestinationAccess);
	}
	RefreshDestinationAccess();
}

void ULastFPSMapScreenWidget::NativeDestruct()
{
	if (ULastFPSQuestSubsystem* QuestSubsystem = ULastFPSQuestSubsystem::Get(this))
	{
		QuestSubsystem->OnQuestStateChanged.RemoveDynamic(
			this, &ULastFPSMapScreenWidget::RefreshDestinationAccess);
	}

	for (ULastFPSTravelEntryButton* Destination : BoundDestinations)
	{
		if (Destination)
		{
			Destination->OnClicked().RemoveAll(this);
		}
	}
	BoundDestinations.Reset();

	Super::NativeDestruct();
}

void ULastFPSMapScreenWidget::RefreshDestinationAccess()
{
	UGameInstance* GameInstance = GetGameInstance();
	const ULastFPSLevelTravelSubsystem* TravelSubsystem = GameInstance
		? GameInstance->GetSubsystem<ULastFPSLevelTravelSubsystem>()
		: nullptr;
	if (!TravelSubsystem)
	{
		return;
	}

	for (ULastFPSTravelEntryButton* Destination : BoundDestinations)
	{
		if (!Destination)
		{
			continue;
		}

		const FLastFPSTravelEntryRequest& Request = Destination->GetTravelRequest();
		Destination->ApplyTravelAccess(
			TravelSubsystem->IsTravelRequestUnlocked(Request),
			TravelSubsystem->GetTravelLockedIcon(Request));
	}
}

FReply ULastFPSMapScreenWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 목적지 버튼 위에서 누른 경우는 버튼이 먼저 처리하므로 여기까지 오지 않는다.
	if (GetTransformTarget() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPanning = true;
		LastScreenPosition = InMouseEvent.GetScreenSpacePosition();

		// 커서가 위젯 밖으로 나가도 드래그가 이어지도록 마우스를 붙잡는다.
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULastFPSMapScreenWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply ULastFPSMapScreenWidget::NativeOnMouseMove(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsPanning || !GetTransformTarget())
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	const FVector2D ScreenDelta = ScreenPosition - LastScreenPosition;
	LastScreenPosition = ScreenPosition;

	// 스크린 이동량을 위젯 로컬 크기로 환산한다. DPI 스케일이 걸려 있어도 커서를 그대로 따라간다.
	const float Scale = FMath::Max(InGeometry.Scale, KINDA_SMALL_NUMBER);
	PanOffset += ScreenDelta / Scale;

	ClampPanOffset(InGeometry);
	ApplyMapTransform();
	return FReply::Handled();
}

FReply ULastFPSMapScreenWidget::NativeOnMouseWheel(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!GetTransformTarget())
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const float WheelDelta = InMouseEvent.GetWheelDelta();
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return FReply::Unhandled();
	}

	const float PreviousZoom = Zoom;
	const float Step = FMath::Max(ZoomStep, 1.01f);
	Zoom = FMath::Clamp(Zoom * FMath::Pow(Step, WheelDelta), MinZoom, MaxZoom);
	if (FMath::IsNearlyEqual(Zoom, PreviousZoom))
	{
		return FReply::Handled();
	}

	// 커서가 가리키던 지점이 제자리에 남도록 이동량을 함께 보정한다.
	// 그렇게 하지 않으면 화면 중심 기준으로 확대돼 보고 있던 곳이 밀려난다.
	const FVector2D LocalCursor = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	PanOffset = LocalCursor - (LocalCursor - PanOffset) * (Zoom / PreviousZoom);

	ClampPanOffset(InGeometry);
	ApplyMapTransform();
	return FReply::Handled();
}

void ULastFPSMapScreenWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	// 캡처가 풀린 채 커서가 빠져나가면 드래그 상태가 남는다.
	bIsPanning = false;
	Super::NativeOnMouseLeave(InMouseEvent);
}

UWidget* ULastFPSMapScreenWidget::GetTransformTarget() const
{
	// 알맹이가 따로 지정돼 있으면 그것만 움직인다. 창은 제자리에 남아 기준 역할을 한다.
	return Panel_MapContent ? Panel_MapContent.Get() : Panel_MapView.Get();
}

FVector2D ULastFPSMapScreenWidget::GetViewSize(const FGeometry& FallbackGeometry) const
{
	// 창 위젯이 지정돼 있으면 그 사각형이 곧 이동 범위다.
	if (Panel_MapContent && Panel_MapView)
	{
		const FVector2D Size = Panel_MapView->GetCachedGeometry().GetLocalSize();
		if (Size.X > KINDA_SMALL_NUMBER && Size.Y > KINDA_SMALL_NUMBER)
		{
			return Size;
		}
	}

	// 알맹이만 있는 구성이면 그것이 놓인 칸을 창으로 본다.
	if (const UWidget* Target = GetTransformTarget())
	{
		if (const UPanelWidget* ViewParent = Target->GetParent())
		{
			const FVector2D Size = ViewParent->GetCachedGeometry().GetLocalSize();
			if (Size.X > KINDA_SMALL_NUMBER && Size.Y > KINDA_SMALL_NUMBER)
			{
				return Size;
			}
		}
	}

	return FallbackGeometry.GetLocalSize();
}

void ULastFPSMapScreenWidget::ApplyMapTransform()
{
	UWidget* Target = GetTransformTarget();
	if (!Target)
	{
		return;
	}

	Target->SetRenderScale(FVector2D(Zoom, Zoom));
	Target->SetRenderTranslation(PanOffset);
}

void ULastFPSMapScreenWidget::ClampPanOffset(const FGeometry& FallbackGeometry)
{
	UWidget* Target = GetTransformTarget();
	if (!bClampPanToView || !Target)
	{
		return;
	}

	const FVector2D ViewSize = GetViewSize(FallbackGeometry);

	// 배치된 실제 크기를 쓴다. GetDesiredSize 는 머티리얼 브러시의 ImageSize(기본 32×32)를
	// 그대로 돌려줄 수 있어 기준으로 삼으면 범위가 엉뚱하게 잡힌다.
	FVector2D LayoutSize = Target->GetCachedGeometry().GetLocalSize();
	if (LayoutSize.X <= KINDA_SMALL_NUMBER || LayoutSize.Y <= KINDA_SMALL_NUMBER)
	{
		LayoutSize = Target->GetDesiredSize();
	}

	const FVector2D ContentSize = LayoutSize * Zoom;

	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		// 지도가 뷰보다 크면 가장자리가 안으로 못 들어오고, 작으면 뷰 밖으로 못 나간다.
		// 두 경우 모두 "0 과 (뷰 - 지도) 사이"라는 같은 구간으로 표현된다.
		const float Slack = ViewSize[Axis] - ContentSize[Axis];
		PanOffset[Axis] = FMath::Clamp(
			PanOffset[Axis], FMath::Min(0.f, Slack), FMath::Max(0.f, Slack));
	}
}

void ULastFPSMapScreenWidget::HandleDestinationClicked(
	TWeakObjectPtr<ULastFPSTravelEntryButton> DestinationButton)
{
	ULastFPSTravelEntryButton* Destination = DestinationButton.Get();
	APlayerController* PlayerController = GetOwningPlayer();
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSLevelTravelSubsystem* TravelSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSLevelTravelSubsystem>() : nullptr;

	if (!Destination || !PlayerController || !TravelSubsystem)
	{
		UE_LOG(LogLastFPSMapScreen, Error,
			TEXT("이동을 시작할 수 없습니다: Widget=%s, Button=%s, PlayerController=%s, TravelSubsystem=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Destination),
			*GetNameSafe(PlayerController),
			*GetNameSafe(TravelSubsystem));
		return;
	}

	const FLastFPSTravelEntryRequest& Request = Destination->GetTravelRequest();
	const ELastFPSTravelRequestResult Result = TravelSubsystem->RequestTravel(PlayerController, Request);
	if (Result == ELastFPSTravelRequestResult::MissingRequiredWeapon)
	{
		if (ALastFPSPlayerController* LastFPSPlayerController =
			Cast<ALastFPSPlayerController>(PlayerController))
		{
			LastFPSPlayerController->ShowNotice(
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NoticeTitle),
				FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::BattleWeaponRequiredBody));
		}
		else
		{
			UE_LOG(LogLastFPSMapScreen, Error,
				TEXT("무기 미장착 Notice를 표시할 수 없습니다: PlayerController=%s"),
				*GetNameSafe(PlayerController));
		}
		return;
	}
	if (Result != ELastFPSTravelRequestResult::Accepted)
	{
		UE_LOG(LogLastFPSMapScreen, Warning,
			TEXT("이동 요청이 거부되었습니다: Widget=%s, Type=%s, BattleDefinition=%s, Result=%s"),
			*GetNameSafe(this),
			*StaticEnum<ELastFPSTravelEntryType>()->GetNameStringByValue(static_cast<int64>(Request.Type)),
			*Request.BattleDefinitionId.ToString(),
			*StaticEnum<ELastFPSTravelRequestResult>()->GetNameStringByValue(static_cast<int64>(Result)));
		return;
	}

	// 이동이 받아들여지면 아웃게임 UI 를 접는다. 껍데기(shell)까지 함께 닫아야 로딩 화면이 가려지지 않는다.
	DeactivateWidget();
}
