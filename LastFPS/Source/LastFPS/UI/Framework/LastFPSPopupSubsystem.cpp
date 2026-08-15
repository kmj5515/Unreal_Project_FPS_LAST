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

ULastFPSPopupSubsystem* ULastFPSPopupSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine || !WorldContext)
	{
		return nullptr;
	}

	const UWorld* World =
		GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	// LocalPlayer 서브시스템이라 "어느 플레이어의 팝업인가"가 정해져야 한다.
	// 화면 분할이 없으므로 첫 로컬 플레이어로 충분하다. 분할이 생기면 호출부가 플레이어를 넘겨야 한다.
	const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();
	return LocalPlayer
		? const_cast<ULocalPlayer*>(LocalPlayer)->GetSubsystem<ULastFPSPopupSubsystem>()
		: nullptr;
}

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
	PendingRequests.Reset();
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

	// 이 태그를 기다리며 쌓여 있던 표시 요청을 여기서 한 번에 연다.
	FlushPendingRequests(PopupTag, /*bAllowOpen=*/true);
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

void ULastFPSPopupSubsystem::RequestPopupInternal(
	const FGameplayTag& PopupTag, UClass* ExpectedClass, FOnPopupReady OnReady)
{
	// 이미 로드돼 있으면(미리 로드 대상이거나 앞서 한 번 열렸으면) 기다릴 이유가 없다.
	if (LoadedWidgetClasses.Contains(PopupTag))
	{
		ULastFPSModalDialogBase* Popup = PushPopupInternal(PopupTag, ExpectedClass);
		if (OnReady)
		{
			OnReady(Popup);
		}
		return;
	}

	const FLastFPSPopupEntry* Entry = FindEntry(PopupTag);
	if (!Entry || !DoesEntryMatchContext(*Entry, ActiveContextTags))
	{
		UE_LOG(
			LogLastFPSPopup,
			Warning,
			TEXT("비동기 요청을 처리할 수 없습니다(미등록이거나 현재 목적지에서 사용 불가): %s"),
			*PopupTag.ToString());
		if (OnReady)
		{
			OnReady(nullptr);
		}
		return;
	}

	// 레벨이 넘어가는 중에 로드를 시작하면 도착 후 엉뚱한 시점에 팝업이 튀어나온다.
	if (bLevelTransitionInProgress)
	{
		if (OnReady)
		{
			OnReady(nullptr);
		}
		return;
	}

	PendingRequests.FindOrAdd(PopupTag).Add({ ExpectedClass, MoveTemp(OnReady) });

	// 같은 태그를 연타해도 로드 핸들은 하나만 돈다. 완료 시 대기 목록을 한 번에 처리한다.
	if (!LoadHandles.Contains(PopupTag))
	{
		BeginLoadEntry(*Entry);
		if (!LoadHandles.Contains(PopupTag))
		{
			// 로드 시작 자체가 실패했으면 대기시키지 않고 즉시 실패를 알린다.
			FlushPendingRequests(PopupTag, /*bAllowOpen=*/false);
		}
	}
}

void ULastFPSPopupSubsystem::FlushPendingRequests(
	const FGameplayTag& PopupTag, const bool bAllowOpen)
{
	TArray<FPendingPopupRequest> Requests;
	if (!PendingRequests.RemoveAndCopyValue(PopupTag, Requests))
	{
		return;
	}

	const bool bCanOpen = bAllowOpen && LoadedWidgetClasses.Contains(PopupTag);
	for (FPendingPopupRequest& Request : Requests)
	{
		ULastFPSModalDialogBase* Popup =
			bCanOpen ? PushPopupInternal(PopupTag, Request.ExpectedClass) : nullptr;

		if (Request.OnReady)
		{
			Request.OnReady(Popup);
		}
	}
}

void ULastFPSPopupSubsystem::ReleaseEntry(
	const FGameplayTag& PopupTag)
{
	// 해제되면 기다리던 요청은 성립하지 않는다. 응답 없이 버리지 않고 실패로 마무리한다.
	FlushPendingRequests(PopupTag, /*bAllowOpen=*/false);

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

ULastFPSModalDialogBase* ULastFPSPopupSubsystem::PushPopupInternal(
	const FGameplayTag& PopupTag, UClass* ExpectedClass)
{
	UClass* LoadedClass = EnsureEntryClassLoaded(PopupTag);
	if (!LoadedClass || !ExpectedClass || !LoadedClass->IsChildOf(ExpectedClass))
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
		
	ULastFPSModalDialogBase* Popup = nullptr;
	if (RootLayout)
	{
		Popup = RootLayout->PushWidgetToLayerStack<ULastFPSModalDialogBase>(LastFPSUITags::Layer_Modal(), LoadedClass);
	}
	else
	{
		UE_LOG(
			LogLastFPSPopup,
			Warning,
			TEXT("PrimaryGameLayout이 준비되지 않아 Viewport에 직접 팝업을 엽니다: %s"),
			*PopupTag.ToString());
			
		if (APlayerController* PC = GetLocalPlayer()->GetPlayerController(GetWorld()))
		{
			Popup = CreateWidget<ULastFPSModalDialogBase>(PC, LoadedClass);
			if (Popup)
			{
				Popup->AddToViewport(100);
				Popup->ActivateWidget();
			}
		}
	}

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

	// 전환 중 도착할 로드 결과로 팝업이 뜨지 않도록 대기 요청을 모두 정리한다.
	TArray<FGameplayTag> PendingTags;
	PendingRequests.GetKeys(PendingTags);
	for (const FGameplayTag& PendingTag : PendingTags)
	{
		FlushPendingRequests(PendingTag, /*bAllowOpen=*/false);
	}

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

	// 자신이 곧 대상 서브시스템이라 Get() 을 거치지 않고 바로 연다.
	ULastFPSConfirmWidget* Popup = Cast<ULastFPSConfirmWidget>(
		PushPopupInternal(LastFPSPopupTags::Confirmation(), ULastFPSConfirmWidget::StaticClass()));
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

	ULastFPSNoticeWidget* Popup = Cast<ULastFPSNoticeWidget>(
		PushPopupInternal(LastFPSPopupTags::Notice(), ULastFPSNoticeWidget::StaticClass()));
	if (!Popup)
	{
		ResultCallback.ExecuteIfBound(ECommonMessagingResult::Unknown);
		return;
	}

	Popup->SetupDialog(DialogDescriptor, MoveTemp(ResultCallback));
}
