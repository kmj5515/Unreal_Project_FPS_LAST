#include "UI/Framework/LastFPSPopupSubsystem.h"

#include "Data/Definitions/LastFPSPopupCatalog.h"
#include "Game/LastFPSGameStateBase.h"
#include "UI/Common/LastFPSConfirmWidget.h"
#include "UI/Common/LastFPSNoticeWidget.h"
#include "UI/Common/LastFPSQuantityDialogWidget.h"
#include "UI/Dialogue/LastFPSDialogueWidget.h"
#include "UI/Framework/LastFPSModalDialogBase.h"
#include "UI/Framework/LastFPSPopupTags.h"
#include "UI/Framework/LastFPSUITags.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Messaging/CommonGameDialog.h"
#include "PrimaryGameLayout.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPopup, Log, All);

void ULastFPSPopupSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadCatalog();
	PreLoadMapDelegateHandle =
		FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
			this,
			&ThisClass::HandlePreLoadMap);

	if (GEngine)
	{
		TravelFailureDelegateHandle =
			GEngine->OnTravelFailure().AddWeakLambda(
				this,
				[this](
					UWorld* World,
					ETravelFailure::Type,
					const FString&)
				{
					const ULocalPlayer* LocalPlayer = GetLocalPlayer();
					if (bLevelTransitionInProgress
						&& World
						&& LocalPlayer
						&& World->GetGameInstance()
							== LocalPlayer->GetGameInstance())
					{
						const FGameplayTagContainer ContextTags =
							ActiveContextTags;
						ApplyDestinationContext(ContextTags);
					}
				});
	}
}

void ULastFPSPopupSubsystem::Deinitialize()
{
	if (PreLoadMapDelegateHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(
			PreLoadMapDelegateHandle);
		PreLoadMapDelegateHandle.Reset();
	}

	if (GEngine && TravelFailureDelegateHandle.IsValid())
	{
		GEngine->OnTravelFailure().Remove(TravelFailureDelegateHandle);
		TravelFailureDelegateHandle.Reset();
	}

	if (ALastFPSGameStateBase* GameState = ContextSource.Get())
	{
		GameState->OnDestinationContextChanged.RemoveAll(this);
	}
	ContextSource.Reset();

	CloseAllActivePopups();
	LoadHandles.Reset();
	LoadedWidgetClasses.Reset();
	EntryIndices.Reset();
	PopupCatalog = nullptr;
	ActiveContextTags.Reset();
	bHasAppliedDestinationContext = false;
	bLevelTransitionInProgress = false;

	Super::Deinitialize();
}

void ULastFPSPopupSubsystem::LoadCatalog()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> CatalogIds;
	AssetManager.GetPrimaryAssetIdList(
		ULastFPSPopupCatalog::PrimaryAssetType,
		CatalogIds);

	if (CatalogIds.IsEmpty())
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("Asset Manager에서 PopupCatalog를 찾지 못했습니다."));
		return;
	}

	CatalogIds.Sort(
		[](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
		{
			return Left.ToString() < Right.ToString();
		});

	if (CatalogIds.Num() > 1)
	{
		UE_LOG(
			LogLastFPSPopup,
			Warning,
			TEXT("PopupCatalog가 여러 개입니다. 첫 번째 에셋을 사용합니다: %s"),
			*CatalogIds[0].ToString());
	}

	const FSoftObjectPath CatalogPath =
		AssetManager.GetPrimaryAssetPath(CatalogIds[0]);
	PopupCatalog = Cast<ULastFPSPopupCatalog>(CatalogPath.TryLoad());
	if (!PopupCatalog)
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("PopupCatalog를 로드하지 못했습니다: %s"),
			*CatalogIds[0].ToString());
		return;
	}

	BuildEntryIndex();
	for (const FLastFPSPopupEntry& Entry : PopupCatalog->Popups)
	{
		if (FindEntry(Entry.PopupTag) == &Entry
			&& Entry.Lifecycle.LoadTiming
			== ELastFPSPopupLoadTiming::LocalPlayerInitialize)
		{
			BeginLoadEntry(Entry);
		}
	}
}

void ULastFPSPopupSubsystem::BuildEntryIndex()
{
	EntryIndices.Reset();
	if (!PopupCatalog)
	{
		return;
	}

	for (int32 Index = 0; Index < PopupCatalog->Popups.Num(); ++Index)
	{
		const FLastFPSPopupEntry& Entry = PopupCatalog->Popups[Index];
		if (!Entry.PopupTag.IsValid() || Entry.WidgetClass.IsNull())
		{
			UE_LOG(
				LogLastFPSPopup,
				Error,
				TEXT("PopupCatalog의 %d번 항목에 필수 값이 없습니다."),
				Index);
			continue;
		}

		if (EntryIndices.Contains(Entry.PopupTag))
		{
			UE_LOG(
				LogLastFPSPopup,
				Error,
				TEXT("PopupTag가 중복되었습니다: %s"),
				*Entry.PopupTag.ToString());
			continue;
		}

		EntryIndices.Add(Entry.PopupTag, Index);
	}
}

const FLastFPSPopupEntry* ULastFPSPopupSubsystem::FindEntry(
	const FGameplayTag& PopupTag) const
{
	if (!PopupCatalog)
	{
		return nullptr;
	}

	const int32* Index = EntryIndices.Find(PopupTag);
	return Index && PopupCatalog->Popups.IsValidIndex(*Index)
		? &PopupCatalog->Popups[*Index]
		: nullptr;
}

bool ULastFPSPopupSubsystem::DoesEntryMatchContext(
	const FLastFPSPopupEntry& Entry,
	const FGameplayTagContainer& ContextTags) const
{
	return Entry.Lifecycle.ContextQuery.IsEmpty()
		|| Entry.Lifecycle.ContextQuery.Matches(ContextTags);
}

void ULastFPSPopupSubsystem::BeginLoadEntry(
	const FLastFPSPopupEntry& Entry)
{
	if (FindEntry(Entry.PopupTag) != &Entry)
	{
		return;
	}

	if (LoadedWidgetClasses.Contains(Entry.PopupTag)
		|| LoadHandles.Contains(Entry.PopupTag))
	{
		return;
	}

	if (UClass* LoadedClass = Entry.WidgetClass.Get())
	{
		LoadedWidgetClasses.Add(Entry.PopupTag, LoadedClass);
		return;
	}

	TSharedPtr<FStreamableHandle> LoadHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			Entry.WidgetClass.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(
				this,
				&ThisClass::HandleEntryLoadCompleted,
				Entry.PopupTag));
	if (!LoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("팝업 비동기 로드를 시작하지 못했습니다: %s"),
			*Entry.PopupTag.ToString());
		return;
	}

	LoadHandles.Add(Entry.PopupTag, MoveTemp(LoadHandle));
}

void ULastFPSPopupSubsystem::HandleEntryLoadCompleted(
	const FGameplayTag PopupTag)
{
	if (!LoadHandles.Contains(PopupTag))
	{
		return;
	}

	const FLastFPSPopupEntry* Entry = FindEntry(PopupTag);
	UClass* LoadedClass = Entry ? Entry->WidgetClass.Get() : nullptr;
	if (!LoadedClass
		|| !LoadedClass->IsChildOf(ULastFPSModalDialogBase::StaticClass()))
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("팝업 클래스 로드 결과가 올바르지 않습니다: %s"),
			*PopupTag.ToString());
		ReleaseEntry(PopupTag);
		return;
	}

	LoadedWidgetClasses.Add(PopupTag, LoadedClass);
}

UClass* ULastFPSPopupSubsystem::EnsureEntryClassLoaded(
	const FGameplayTag& PopupTag)
{
	if (TObjectPtr<UClass>* LoadedClass =
		LoadedWidgetClasses.Find(PopupTag))
	{
		return LoadedClass->Get();
	}

	const FLastFPSPopupEntry* Entry = FindEntry(PopupTag);
	if (!Entry)
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("PopupCatalog에 PopupTag가 없습니다: %s"),
			*PopupTag.ToString());
		return nullptr;
	}

	if (!DoesEntryMatchContext(*Entry, ActiveContextTags))
	{
		UE_LOG(
			LogLastFPSPopup,
			Warning,
			TEXT("현재 목적지에서 사용할 수 없는 팝업입니다: %s"),
			*PopupTag.ToString());
		return nullptr;
	}

	UClass* LoadedClass = Entry->WidgetClass.LoadSynchronous();
	if (!LoadedClass
		|| !LoadedClass->IsChildOf(ULastFPSModalDialogBase::StaticClass()))
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("팝업 클래스를 로드하지 못했습니다: %s"),
			*PopupTag.ToString());
		return nullptr;
	}

	LoadedWidgetClasses.Add(PopupTag, LoadedClass);
	return LoadedClass;
}

void ULastFPSPopupSubsystem::ReleaseEntry(
	const FGameplayTag& PopupTag)
{
	if (TSharedPtr<FStreamableHandle>* LoadHandle =
		LoadHandles.Find(PopupTag))
	{
		if (LoadHandle->IsValid() && (*LoadHandle)->IsActive())
		{
			(*LoadHandle)->CancelHandle();
		}
		LoadHandles.Remove(PopupTag);
	}

	LoadedWidgetClasses.Remove(PopupTag);
}

template<typename TPopupWidget>
TPopupWidget* ULastFPSPopupSubsystem::PushPopup(
	const FGameplayTag& PopupTag)
{
	UClass* LoadedClass = EnsureEntryClassLoaded(PopupTag);
	if (!LoadedClass || !LoadedClass->IsChildOf(TPopupWidget::StaticClass()))
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("요청한 팝업 타입과 등록된 클래스가 다릅니다: %s"),
			*PopupTag.ToString());
		return nullptr;
	}

	UPrimaryGameLayout* RootLayout =
		UPrimaryGameLayout::GetPrimaryGameLayout(GetLocalPlayer());
	if (!RootLayout)
	{
		UE_LOG(
			LogLastFPSPopup,
			Warning,
			TEXT("PrimaryGameLayout이 준비되지 않아 팝업을 열 수 없습니다: %s"),
			*PopupTag.ToString());
		return nullptr;
	}

	TPopupWidget* Popup =
		RootLayout->PushWidgetToLayerStack<TPopupWidget>(
			LastFPSUITags::Layer_Modal(),
			LoadedClass);
	if (Popup)
	{
		ActivePopups.FindOrAdd(PopupTag).Add(Popup);
		Popup->OnDeactivated().AddUObject(
			this,
			&ThisClass::HandlePopupDeactivated,
			PopupTag,
			TWeakObjectPtr<ULastFPSModalDialogBase>(Popup));
	}
	return Popup;
}

void ULastFPSPopupSubsystem::CloseActivePopup(
	const FGameplayTag& PopupTag)
{
	TArray<TWeakObjectPtr<ULastFPSModalDialogBase>> Popups;
	if (!ActivePopups.RemoveAndCopyValue(PopupTag, Popups))
	{
		return;
	}

	for (const TWeakObjectPtr<ULastFPSModalDialogBase>& WeakPopup : Popups)
	{
		if (ULastFPSModalDialogBase* Popup = WeakPopup.Get())
		{
			Popup->KillDialog();
		}
	}
}

void ULastFPSPopupSubsystem::HandlePopupDeactivated(
	const FGameplayTag PopupTag,
	const TWeakObjectPtr<ULastFPSModalDialogBase> Popup)
{
	if (ULastFPSModalDialogBase* PopupWidget = Popup.Get())
	{
		PopupWidget->OnDeactivated().RemoveAll(this);
	}

	TArray<TWeakObjectPtr<ULastFPSModalDialogBase>>* Popups =
		ActivePopups.Find(PopupTag);
	if (!Popups)
	{
		return;
	}

	Popups->RemoveAll(
		[Popup](const TWeakObjectPtr<ULastFPSModalDialogBase>& Candidate)
		{
			return !Candidate.IsValid() || Candidate == Popup;
		});
	if (Popups->IsEmpty())
	{
		ActivePopups.Remove(PopupTag);
	}
}

void ULastFPSPopupSubsystem::CloseAllActivePopups()
{
	TArray<FGameplayTag> PopupTags;
	ActivePopups.GetKeys(PopupTags);
	for (const FGameplayTag& PopupTag : PopupTags)
	{
		CloseActivePopup(PopupTag);
	}
}

void ULastFPSPopupSubsystem::CloseEntriesForLevelTransition()
{
	if (!PopupCatalog)
	{
		return;
	}

	for (const FLastFPSPopupEntry& Entry : PopupCatalog->Popups)
	{
		if (FindEntry(Entry.PopupTag) == &Entry
			&& Entry.Lifecycle.InstanceCloseTiming
			== ELastFPSPopupInstanceCloseTiming::LevelTransitionStart)
		{
			CloseActivePopup(Entry.PopupTag);
		}
	}
}

void ULastFPSPopupSubsystem::ReleaseEntriesForLevelTransition()
{
	if (!PopupCatalog)
	{
		return;
	}

	for (const FLastFPSPopupEntry& Entry : PopupCatalog->Popups)
	{
		if (FindEntry(Entry.PopupTag) == &Entry
			&& Entry.Lifecycle.AssetReleaseTiming
			== ELastFPSPopupAssetReleaseTiming::LevelTransitionStart)
		{
			ReleaseEntry(Entry.PopupTag);
		}
	}
}

void ULastFPSPopupSubsystem::ApplyDestinationContext(
	const FGameplayTagContainer& NewContextTags)
{
	if (!PopupCatalog)
	{
		return;
	}

	const FGameplayTagContainer PreviousContextTags = ActiveContextTags;
	const bool bHadPreviousContext = bHasAppliedDestinationContext;
	const bool bForceDestinationEnter = bLevelTransitionInProgress;
	ActiveContextTags = NewContextTags;
	bHasAppliedDestinationContext = true;
	bLevelTransitionInProgress = false;

	for (const FLastFPSPopupEntry& Entry : PopupCatalog->Popups)
	{
		if (FindEntry(Entry.PopupTag) != &Entry)
		{
			continue;
		}

		const bool bMatchedPreviousContext =
			bHadPreviousContext
			&& DoesEntryMatchContext(Entry, PreviousContextTags);
		const bool bMatchesNewContext =
			DoesEntryMatchContext(Entry, NewContextTags);

		if (bMatchedPreviousContext && !bMatchesNewContext)
		{
			if (Entry.Lifecycle.InstanceCloseTiming
				== ELastFPSPopupInstanceCloseTiming::DestinationExit)
			{
				CloseActivePopup(Entry.PopupTag);
			}

			if (Entry.Lifecycle.AssetReleaseTiming
				== ELastFPSPopupAssetReleaseTiming::DestinationExit)
			{
				ReleaseEntry(Entry.PopupTag);
			}
		}

		if ((bForceDestinationEnter
				|| !bHadPreviousContext
				|| !bMatchedPreviousContext)
			&& bMatchesNewContext
			&& Entry.Lifecycle.LoadTiming
				== ELastFPSPopupLoadTiming::DestinationEnter)
		{
			BeginLoadEntry(Entry);
		}
	}
}

void ULastFPSPopupSubsystem::BindDestinationContextSource(
	ALastFPSGameStateBase* GameState)
{
	if (ContextSource.Get() == GameState)
	{
		return;
	}

	if (ALastFPSGameStateBase* PreviousSource = ContextSource.Get())
	{
		PreviousSource->OnDestinationContextChanged.RemoveAll(this);
	}

	ContextSource = GameState;
	if (!GameState)
	{
		return;
	}

	GameState->OnDestinationContextChanged.AddUObject(
		this,
		&ThisClass::HandleDestinationContextChanged);
	ApplyDestinationContext(GameState->GetDestinationContextTags());
}

void ULastFPSPopupSubsystem::HandleDestinationContextChanged(
	const FGameplayTagContainer& NewContextTags)
{
	ApplyDestinationContext(NewContextTags);
}

void ULastFPSPopupSubsystem::HandlePreLoadMap(
	const FWorldContext& WorldContext,
	const FString& MapName)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer
		|| WorldContext.OwningGameInstance
			!= LocalPlayer->GetGameInstance())
	{
		return;
	}

	CloseEntriesForLevelTransition();
	ReleaseEntriesForLevelTransition();
	bLevelTransitionInProgress = true;
}

void ULastFPSPopupSubsystem::ShowConfirmation(
	UCommonGameDialogDescriptor* DialogDescriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	if (!DialogDescriptor)
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("확인 팝업 Descriptor가 유효하지 않습니다."));
		ResultCallback.ExecuteIfBound(ECommonMessagingResult::Unknown);
		return;
	}

	ULastFPSConfirmWidget* Popup =
		PushPopup<ULastFPSConfirmWidget>(
			LastFPSPopupTags::Confirmation());
	if (!Popup)
	{
		ResultCallback.ExecuteIfBound(ECommonMessagingResult::Unknown);
		return;
	}

	Popup->SetupDialog(DialogDescriptor, MoveTemp(ResultCallback));
}

void ULastFPSPopupSubsystem::ShowError(
	UCommonGameDialogDescriptor* DialogDescriptor,
	FCommonMessagingResultDelegate ResultCallback)
{
	if (!DialogDescriptor)
	{
		UE_LOG(
			LogLastFPSPopup,
			Error,
			TEXT("공지 팝업 Descriptor가 유효하지 않습니다."));
		ResultCallback.ExecuteIfBound(ECommonMessagingResult::Unknown);
		return;
	}

	ULastFPSNoticeWidget* Popup =
		PushPopup<ULastFPSNoticeWidget>(
			LastFPSPopupTags::Notice());
	if (!Popup)
	{
		ResultCallback.ExecuteIfBound(ECommonMessagingResult::Unknown);
		return;
	}

	Popup->SetupDialog(DialogDescriptor, MoveTemp(ResultCallback));
}

ULastFPSQuantityDialogWidget*
ULastFPSPopupSubsystem::ShowQuantityPrompt(
	const FText& Title,
	const FText& ItemName,
	const int32 UnitPrice,
	const int32 MaxQuantity)
{
	ULastFPSQuantityDialogWidget* Popup =
		PushPopup<ULastFPSQuantityDialogWidget>(
			LastFPSPopupTags::Quantity());
	if (Popup)
	{
		Popup->SetupQuantity(
			Title,
			ItemName,
			UnitPrice,
			MaxQuantity);
	}
	return Popup;
}

ULastFPSDialogueWidget* ULastFPSPopupSubsystem::ShowDialogue(
	const FText& Speaker,
	const TArray<FText>& Lines)
{
	ULastFPSDialogueWidget* Popup =
		PushPopup<ULastFPSDialogueWidget>(
			LastFPSPopupTags::Dialogue());
	if (Popup)
	{
		Popup->SetupDialogue(Speaker, Lines);
	}
	return Popup;
}
