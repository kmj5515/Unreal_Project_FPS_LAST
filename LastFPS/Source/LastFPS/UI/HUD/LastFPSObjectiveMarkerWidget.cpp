#include "UI/HUD/LastFPSObjectiveMarkerWidget.h"

#include "UI/HUD/LastFPSObjectiveMarkerEntryWidget.h"
#include "Quest/LastFPSQuestSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void ULastFPSObjectiveMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshMarkers();
}

ULastFPSQuestSubsystem* ULastFPSObjectiveMarkerWidget::GetQuestSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<ULastFPSQuestSubsystem>();
		}
	}
	return nullptr;
}

void ULastFPSObjectiveMarkerWidget::RefreshMarkers()
{
	if (!Canvas_Markers || !EntryWidgetClass)
	{
		return;
	}

	ULastFPSQuestSubsystem* Quest = GetQuestSubsystem();
	APlayerController* PC = GetOwningPlayer();

	TArray<FLastFPSObjectiveWaypoint> Waypoints;
	if (Quest && PC)
	{
		Quest->GetActiveWaypoints(Waypoints);
	}

	// 거리 기준 위치(플레이어 폰, 없으면 카메라).
	FVector ViewerLocation = FVector::ZeroVector;
	if (PC)
	{
		if (const APawn* Pawn = PC->GetPawn())
		{
			ViewerLocation = Pawn->GetActorLocation();
		}
		else
		{
			FRotator ViewRot;
			PC->GetPlayerViewPoint(ViewerLocation, ViewRot);
		}
	}

	int32 SizeX = 0, SizeY = 0;
	if (PC)
	{
		PC->GetViewportSize(SizeX, SizeY);
	}
	const FVector2D ViewportSize(static_cast<float>(SizeX), static_cast<float>(SizeY));
	const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
	const float SafeDPI = DPI > KINDA_SMALL_NUMBER ? DPI : 1.f;

	// 엔트리 풀을 웨이포인트 수만큼 확보.
	while (MarkerEntries.Num() < Waypoints.Num())
	{
		ULastFPSObjectiveMarkerEntryWidget* Entry = CreateWidget<ULastFPSObjectiveMarkerEntryWidget>(this, EntryWidgetClass);
		if (!Entry)
		{
			break;
		}
		Canvas_Markers->AddChildToCanvas(Entry);
		MarkerEntries.Add(Entry);
	}

	for (int32 i = 0; i < MarkerEntries.Num(); ++i)
	{
		ULastFPSObjectiveMarkerEntryWidget* Entry = MarkerEntries[i];
		if (!Entry)
		{
			continue;
		}

		if (i >= Waypoints.Num() || !PC)
		{
			Entry->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FLastFPSObjectiveWaypoint& Waypoint = Waypoints[i];

		FVector2D ScreenPos = FVector2D::ZeroVector;
		bool bOffScreen = false;
		float AngleDeg = 0.f;
		ComputeScreenPosition(PC, Waypoint.WorldLocation, ViewportSize, EdgeMargin, ScreenPos, bOffScreen, AngleDeg);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Entry->Slot))
		{
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(ScreenPos / SafeDPI); // 픽셀 → 캔버스(슬레이트) 단위
		}

		const float DistanceMeters = FVector::Dist(ViewerLocation, Waypoint.WorldLocation) / 100.f;
		Entry->SetVisibility(ESlateVisibility::HitTestInvisible);
		Entry->UpdateMarker(DistanceMeters, bOffScreen, AngleDeg, Waypoint.Label);
	}
}

void ULastFPSObjectiveMarkerWidget::ComputeScreenPosition(
	APlayerController* PC,
	const FVector& WorldLocation,
	const FVector2D& ViewportSize,
	float Margin,
	FVector2D& OutScreenPos,
	bool& bOutOffScreen,
	float& OutAngleDeg)
{
	FVector2D Projected = FVector2D::ZeroVector;
	const bool bProjected = PC->ProjectWorldLocationToScreen(WorldLocation, Projected, /*bPlayerViewportRelative=*/false);

	FVector CamLocation;
	FRotator CamRotation;
	PC->GetPlayerViewPoint(CamLocation, CamRotation);
	const bool bBehind = FVector::DotProduct(CamRotation.Vector(), WorldLocation - CamLocation) < 0.f;

	const FVector2D Center = ViewportSize * 0.5f;

	// 화면 안(카메라 앞 + 여백 내부) → 그대로 표시.
	if (bProjected && !bBehind
		&& Projected.X >= Margin && Projected.X <= ViewportSize.X - Margin
		&& Projected.Y >= Margin && Projected.Y <= ViewportSize.Y - Margin)
	{
		OutScreenPos = Projected;
		bOutOffScreen = false;
		OutAngleDeg = 0.f;
		return;
	}

	// 화면 밖 → 중심에서의 방향으로 가장자리에 클램프.
	FVector2D Dir = Projected - Center;
	if (bBehind)
	{
		Dir = -Dir; // 카메라 뒤면 투영이 반사되므로 방향 반전.
	}
	if (Dir.IsNearlyZero())
	{
		Dir = FVector2D(0.f, -1.f);
	}
	Dir.Normalize();

	const FVector2D Half = Center - FVector2D(Margin, Margin);
	const float ScaleX = Half.X / FMath::Max(FMath::Abs(Dir.X), KINDA_SMALL_NUMBER);
	const float ScaleY = Half.Y / FMath::Max(FMath::Abs(Dir.Y), KINDA_SMALL_NUMBER);
	const float EdgeScale = FMath::Min(ScaleX, ScaleY);

	OutScreenPos = Center + Dir * EdgeScale;
	bOutOffScreen = true;
	OutAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
}
