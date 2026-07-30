#include "UI/Framework/LastFPSModalDialogBase.h"

#include "UI/Framework/LastFPSPrimaryGameLayout.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "PrimaryGameLayout.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

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
	DeactivateWidget();
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

	if (ActivateSound)
	{
		UGameplayStatics::PlaySound2D(this, ActivateSound);
	}

	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULastFPSModalDialogBase::NativeOnDeactivated()
{
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
