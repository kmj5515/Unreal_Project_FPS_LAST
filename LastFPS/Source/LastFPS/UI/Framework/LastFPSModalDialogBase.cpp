#include "UI/Framework/LastFPSModalDialogBase.h"

#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PrimaryGameLayout.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSModalDialog, Log, All);

ULastFPSModalDialogBase::ULastFPSModalDialogBase()
	: Super()
{
	bIsBackHandler = false;
}

void ULastFPSModalDialogBase::SetDialogText(const FText& InTitle, const FText& InBody)
{
	if (Text_Title)
	{
		Text_Title->SetText(InTitle);
	}
	if (Text_Body)
	{
		Text_Body->SetText(InBody);
	}
}

void ULastFPSModalDialogBase::SetupDialog(
	UCommonGameDialogDescriptor* Descriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	PendingResultCallback = MoveTemp(ResultCallback);
	bResultCompleted = false;

	if (!Descriptor)
	{
		CompleteDialog(ECommonMessagingResult::Unknown);
		return;
	}

	SetDialogText(Descriptor->Header, Descriptor->Body);
}

void ULastFPSModalDialogBase::KillDialog()
{
	CompleteDialog(ECommonMessagingResult::Killed);
	DeactivateWithAnimation();
}

TOptional<FUIInputConfig> ULastFPSModalDialogBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void ULastFPSModalDialogBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bIsBackHandler = false;
}

void ULastFPSModalDialogBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	CancelOutAnimationFallback();
	bWaitingForOutAnimation = false;
	SetIsEnabled(true);

	if (OutAnimation && IsAnimationPlaying(OutAnimation))
	{
		StopAnimation(OutAnimation);
	}

	if (InAnimation)
	{
		PlayAnimation(InAnimation);
	}

	if (ActivateSound)
	{
		UGameplayStatics::PlaySound2D(this, ActivateSound);
	}

	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULastFPSModalDialogBase::NativeOnDeactivated()
{
	CancelOutAnimationFallback();
	bWaitingForOutAnimation = false;

	if (!bResultCompleted && PendingResultCallback.IsBound())
	{
		CompleteDialog(ECommonMessagingResult::Killed);
	}

	Super::NativeOnDeactivated();

	if (DeactivateSound)
	{
		UGameplayStatics::PlaySound2D(this, DeactivateSound);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<APlayerController> WeakController = GetOwningPlayer();
	World->GetTimerManager().SetTimerForNextTick([WeakController]()
	{
		APlayerController* Controller = WeakController.Get();
		if (!Controller)
		{
			return;
		}

		ULastFPSPrimaryGameLayout* Layout = Cast<ULastFPSPrimaryGameLayout>(
			UPrimaryGameLayout::GetPrimaryGameLayout(Controller));
		if (Layout)
		{
			Layout->RestoreFocusToTopActiveWidget();
		}
	});
}

void ULastFPSModalDialogBase::OnAnimationFinished_Implementation(
	const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (bWaitingForOutAnimation && Animation == OutAnimation)
	{
		CancelOutAnimationFallback();
		bWaitingForOutAnimation = false;
		DeactivateWidget();
	}
}

FReply ULastFPSModalDialogBase::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (bCloseOnEscape && InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (NativeOnHandleBackAction())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULastFPSModalDialogBase::CompleteDialog(
	const ECommonMessagingResult Result)
{
	if (bResultCompleted)
	{
		return;
	}

	bResultCompleted = true;
	FCommonMessagingResultDelegate Callback = MoveTemp(PendingResultCallback);
	Callback.ExecuteIfBound(Result);
}

void ULastFPSModalDialogBase::DeactivateWithAnimation()
{
	if (bWaitingForOutAnimation)
	{
		return;
	}

	if (!OutAnimation)
	{
		DeactivateWidget();
		return;
	}

	bWaitingForOutAnimation = true;
	SetIsEnabled(false);
	if (InAnimation && IsAnimationPlaying(InAnimation))
	{
		StopAnimation(InAnimation);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(
			LogLastFPSModalDialog,
			Warning,
			TEXT("모달 '%s'의 종료 애니메이션을 재생할 월드가 없어 즉시 비활성화합니다. Animation=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OutAnimation));
		bWaitingForOutAnimation = false;
		DeactivateWidget();
		return;
	}

	const float AnimationDuration = FMath::Max(
		0.0f,
		OutAnimation->GetEndTime() - OutAnimation->GetStartTime());
	const float FallbackDelay = FMath::Max(
		KINDA_SMALL_NUMBER,
		AnimationDuration + FMath::Max(0.0f, OutAnimationFallbackPaddingSeconds));
	World->GetTimerManager().SetTimer(
		OutAnimationFallbackTimerHandle,
		this,
		&ThisClass::HandleOutAnimationFallbackElapsed,
		FallbackDelay,
		false);
	PlayAnimation(OutAnimation);
}

void ULastFPSModalDialogBase::CancelOutAnimationFallback()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OutAnimationFallbackTimerHandle);
	}
	OutAnimationFallbackTimerHandle.Invalidate();
}

void ULastFPSModalDialogBase::HandleOutAnimationFallbackElapsed()
{
	OutAnimationFallbackTimerHandle.Invalidate();
	if (!bWaitingForOutAnimation)
	{
		return;
	}

	UE_LOG(
		LogLastFPSModalDialog,
		Warning,
		TEXT("모달 '%s'의 종료 애니메이션 완료 이벤트가 오지 않아 안전 타이머로 비활성화합니다. Animation=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OutAnimation));
	bWaitingForOutAnimation = false;
	DeactivateWidget();
}
