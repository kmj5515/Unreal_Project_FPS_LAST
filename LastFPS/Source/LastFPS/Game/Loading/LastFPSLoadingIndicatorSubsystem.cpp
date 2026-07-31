#include "Game/Loading/LastFPSLoadingIndicatorSubsystem.h"

#include "PrimaryGameLayout.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Framework/LastFPSUITags.h"
#include "UI/Loading/LastFPSLoadingIndicatorWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSLoadingIndicatorSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLoadingIndicator, Log, All);

void ULastFPSLoadingIndicatorSubsystem::Deinitialize()
{
	HideInternal();
	Super::Deinitialize();
}

void ULastFPSLoadingIndicatorSubsystem::Show(
	UObject* WorldContextObject,
	APlayerController* OwningPlayer,
	const FText& StatusText)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (ULastFPSLoadingIndicatorSubsystem* Subsystem = GameInstance
		? GameInstance->GetSubsystem<ULastFPSLoadingIndicatorSubsystem>()
		: nullptr)
	{
		Subsystem->ShowInternal(OwningPlayer, StatusText);
	}
}

void ULastFPSLoadingIndicatorSubsystem::Hide(UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (ULastFPSLoadingIndicatorSubsystem* Subsystem = GameInstance
		? GameInstance->GetSubsystem<ULastFPSLoadingIndicatorSubsystem>()
		: nullptr)
	{
		Subsystem->HideInternal();
	}
}

void ULastFPSLoadingIndicatorSubsystem::ShowInternal(
	APlayerController* OwningPlayer,
	const FText& StatusText)
{
	if (IsRunningDedicatedServer() || !IsValid(OwningPlayer) || !OwningPlayer->IsLocalController())
	{
		return;
	}

	UPrimaryGameLayout* RootLayout =
		UPrimaryGameLayout::GetPrimaryGameLayout(OwningPlayer);
	if (!RootLayout)
	{
		UE_LOG(
			LogLastFPSLoadingIndicator,
			Warning,
			TEXT("로딩 인디케이터를 표시할 PrimaryGameLayout이 없습니다. Player=%s"),
			*GetNameSafe(OwningPlayer));
		return;
	}

	if (!IsValid(IndicatorWidget))
	{
		UClass* WidgetClass = IndicatorWidgetClass
			? IndicatorWidgetClass.Get()
			: ULastFPSLoadingIndicatorWidget::StaticClass();
		IndicatorWidget =
			RootLayout->PushWidgetToLayerStack<ULastFPSLoadingIndicatorWidget>(
				LastFPSUITags::Layer_Overlay(),
				WidgetClass,
				[StatusText](ULastFPSLoadingIndicatorWidget& WidgetToInitialize)
				{
					WidgetToInitialize.SetStatusText(StatusText);
				});
	}
	else
	{
		IndicatorWidget->SetStatusText(StatusText);
	}

	if (!IndicatorWidget)
	{
		UE_LOG(
			LogLastFPSLoadingIndicator,
			Error,
			TEXT("로딩 인디케이터 생성에 실패했습니다. GameInstance=%s"),
			*GetNameSafe(GetGameInstance()));
		return;
	}

}

void ULastFPSLoadingIndicatorSubsystem::HideInternal()
{
	if (IsValid(IndicatorWidget))
	{
		IndicatorWidget->DeactivateWidget();
	}
	IndicatorWidget = nullptr;
}

bool ULastFPSLoadingIndicatorSubsystem::IsIndicatorVisible() const
{
	return IsValid(IndicatorWidget) && IndicatorWidget->IsActivated();
}
