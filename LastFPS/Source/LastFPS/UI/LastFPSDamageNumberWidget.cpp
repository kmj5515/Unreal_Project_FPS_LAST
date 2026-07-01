#include "UI/LastFPSDamageNumberWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TextBlock.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

ULastFPSDamageNumberWidget::ULastFPSDamageNumberWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULastFPSDamageNumberWidget::InitializeDamageNumber(
	float DamageAmount,
	float TotalDamageDealt,
	AActor* DamageTargetActor,
	const FVector& FallbackWorldLocation,
	const FVector& TargetWorldOffset,
	const FVector2D& ScreenOffset,
	const FVector2D& RandomScreenOffset,
	bool bCritical)
{
	(void)TotalDamageDealt;

	TrackedTargetActor = DamageTargetActor;
	FallbackDamageWorldLocation = FallbackWorldLocation;
	TrackedWorldOffset = TargetWorldOffset;
	DamageScreenOffset = ScreenOffset;
	DamageRandomScreenOffset = RandomScreenOffset;
	ElapsedTime = 0.f;
	bIsCritical = bCritical;

	EnsureNativeWidgets();
	ApplyDamageText(DamageAmount);
	if (!UpdateScreenPosition())
	{
		RemoveFromParent();
		return;
	}

	PlayDamageNumberAnimation(bIsCritical);
}

void ULastFPSDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureNativeWidgets();
}

void ULastFPSDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!UpdateScreenPosition())
	{
		RemoveFromParent();
		return;
	}

	UpdateLifetime(InDeltaTime);
}

void ULastFPSDamageNumberWidget::PlayDamageNumberAnimation(bool bCritical)
{
	if (bCritical)
	{
		PlayAnimationForward(CriticalAnimation);	
	}
	else
	{
		PlayAnimationForward(GeneralAnimation);
	}
}

void ULastFPSDamageNumberWidget::EnsureNativeWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!DamageTextBlock && WidgetTree->RootWidget == nullptr)
	{
		DamageTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageTextBlock"));
		WidgetTree->RootWidget = DamageTextBlock;

		DamageTextBlock->SetJustification(ETextJustify::Center);
	}
}

void ULastFPSDamageNumberWidget::ApplyDamageText(float DamageAmount)
{
	if (!DamageTextBlock)
	{
		return;
	}

	DamageTextBlock->SetText(FText::AsNumber(FMath::RoundToInt(DamageAmount)));
}

bool ULastFPSDamageNumberWidget::UpdateScreenPosition()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return false;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(ViewportScale, KINDA_SMALL_NUMBER);
	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return false;
	}

	const FVector TargetWorldLocation = TrackedTargetActor.IsValid()
		? TrackedTargetActor->GetActorLocation()
		: FallbackDamageWorldLocation;
	if (TargetWorldLocation.IsNearlyZero())
	{
		return false;
	}

	const FVector TargetDisplayWorldLocation = TargetWorldLocation + TrackedWorldOffset;
	if (PC->PlayerCameraManager)
	{
		const FVector CameraToTarget = TargetDisplayWorldLocation - PC->PlayerCameraManager->GetCameraLocation();
		const FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
		if (FVector::DotProduct(CameraToTarget, CameraForward) <= 0.f)
		{
			return false;
		}
	}

	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PC,
		TargetDisplayWorldLocation,
		ScreenPosition,
		true))
	{
		return false;
	}

	ScreenPosition += DamageScreenOffset + DamageRandomScreenOffset;
	if (!IsScreenPositionVisible(ScreenPosition, ViewportSize))
	{
		return false;
	}

	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetPositionInViewport(ScreenPosition, false);
	return true;
}

bool ULastFPSDamageNumberWidget::IsScreenPositionVisible(
	const FVector2D& ScreenPosition,
	const FVector2D& ViewportSize) const
{
	return ScreenPosition.X >= -ScreenVisibilityPadding
		&& ScreenPosition.Y >= -ScreenVisibilityPadding
		&& ScreenPosition.X <= ViewportSize.X + ScreenVisibilityPadding
		&& ScreenPosition.Y <= ViewportSize.Y + ScreenVisibilityPadding;
}

void ULastFPSDamageNumberWidget::UpdateLifetime(float DeltaTime)
{
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= LifeTime)
	{
		RemoveFromParent();
	}
}
