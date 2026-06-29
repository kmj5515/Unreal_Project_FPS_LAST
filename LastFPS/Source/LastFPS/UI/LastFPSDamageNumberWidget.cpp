#include "UI/LastFPSDamageNumberWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
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
	UpdateScreenPosition();
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

	UpdateScreenPosition();
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

void ULastFPSDamageNumberWidget::UpdateScreenPosition()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(ViewportScale, KINDA_SMALL_NUMBER);
	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return;
	}

	const FVector TargetWorldLocation = TrackedTargetActor.IsValid()
		? TrackedTargetActor->GetActorLocation()
		: FallbackDamageWorldLocation;

	FVector2D ScreenPosition = ViewportSize * 0.5f;

	if (!TargetWorldLocation.IsNearlyZero())
	{
		FVector2D ProjectedWidgetPosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PC,
			TargetWorldLocation + TrackedWorldOffset,
			ProjectedWidgetPosition,
			true))
		{
			ScreenPosition = ProjectedWidgetPosition;
		}
	}

	ScreenPosition += DamageScreenOffset + DamageRandomScreenOffset;
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetPositionInViewport(ScreenPosition, false);
}

void ULastFPSDamageNumberWidget::UpdateLifetime(float DeltaTime)
{
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= LifeTime)
	{
		RemoveFromParent();
	}
}
