#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Messaging/CommonMessagingSubsystem.h"
// ShowPopup 본문이 이 헤더에 있어(어느 .cpp 에서든 호출 가능하도록) 기반 팝업 타입이 완전해야 한다.
#include "UI/Framework/LastFPSModalDialogBase.h"
#include "LastFPSPopupSubsystem.generated.h"

class ALastFPSGameStateBase;
class UClass;
class ULastFPSPopupCatalog;
struct FLastFPSPopupEntry;
struct FStreamableHandle;
struct FWorldContext;

/**
 * 단일 PopupCatalog를 소비하며 팝업별 수명 정책에 따라 클래스와 인스턴스를 관리한다.
 */
UCLASS()
class LASTFPS_API ULastFPSPopupSubsystem : public UCommonMessagingSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void ShowConfirmation(
		UCommonGameDialogDescriptor* DialogDescriptor,
		FCommonMessagingResultDelegate ResultCallback =
			FCommonMessagingResultDelegate()) override;

	virtual void ShowError(
		UCommonGameDialogDescriptor* DialogDescriptor,
		FCommonMessagingResultDelegate ResultCallback =
			FCommonMessagingResultDelegate()) override;

	/** WorldContext 가 속한 LocalPlayer 의 팝업 서브시스템. 없으면 nullptr. */
	static ULastFPSPopupSubsystem* Get(const UObject* WorldContext);
	
	template<typename TPopupWidget>
	static TPopupWidget* ShowPopup(const UObject* WorldContext, const FGameplayTag& PopupTag);
	
	template<typename TPopupWidget>
	static void ShowPopupAsync(const UObject* WorldContext,const FGameplayTag& PopupTag,
		TFunction<void(TPopupWidget*)> OnReady);

	void BindDestinationContextSource(ALastFPSGameStateBase* GameState);

	void ApplyDestinationContext(const FGameplayTagContainer& NewContextTags);

private:
	/** 준비된 팝업(또는 실패 시 nullptr)을 받는 콜백. 템플릿 밖에서 다루려고 기반 타입으로 받는다. */
	using FOnPopupReady = TFunction<void(ULastFPSModalDialogBase*)>;

	/** 로드가 끝나기를 기다리는 요청 1건. 태그별로 여러 건이 쌓일 수 있다. */
	struct FPendingPopupRequest
	{
		TObjectPtr<UClass> ExpectedClass;
		FOnPopupReady OnReady;
	};

	ULastFPSModalDialogBase* PushPopupInternal(const FGameplayTag& PopupTag, UClass* ExpectedClass);

	/**
	 * 클래스가 준비돼 있으면 즉시 열고, 아니면 비동기 로드를 걸어 완료 후 연다.
	 * 템플릿 밖에 두어 코드가 팝업 타입 수만큼 복제되지 않게 한다.
	 */
	void RequestPopupInternal(
		const FGameplayTag& PopupTag,
		UClass* ExpectedClass,
		FOnPopupReady OnReady);

	/**
	 * 대기 중인 요청을 모두 마무리한다. 응답 없이 버려지는 요청이 없도록 실패도 통보한다.
	 * @param bAllowOpen false 면 팝업을 열지 않고 전부 nullptr 로 응답한다(해제·전환 등).
	 */
	void FlushPendingRequests(const FGameplayTag& PopupTag, bool bAllowOpen);
	
	void LoadCatalog();
	void BuildEntryIndex();
	const FLastFPSPopupEntry* FindEntry(const FGameplayTag& PopupTag) const;
	bool DoesEntryMatchContext(const FLastFPSPopupEntry& Entry, const FGameplayTagContainer& ContextTags) const;
	void BeginLoadEntry(const FLastFPSPopupEntry& Entry);
	void HandleEntryLoadCompleted(FGameplayTag PopupTag);
	UClass* EnsureEntryClassLoaded(const FGameplayTag& PopupTag);
	void ReleaseEntry(const FGameplayTag& PopupTag);
	void CloseActivePopup(const FGameplayTag& PopupTag);
	void HandlePopupDeactivated(FGameplayTag PopupTag,TWeakObjectPtr<ULastFPSModalDialogBase> Popup);
	void CloseAllActivePopups();
	void CloseEntriesForLevelTransition();
	void ReleaseEntriesForLevelTransition();
	void HandlePreLoadMap(const FWorldContext& WorldContext,const FString& MapName);
	void HandleDestinationContextChanged(const FGameplayTagContainer& NewContextTags);

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSPopupCatalog> PopupCatalog;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UClass>> LoadedWidgetClasses;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveContextTags;

	TMap<FGameplayTag, int32> EntryIndices;
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> LoadHandles;
	/** 로드 완료를 기다리는 표시 요청. 같은 태그를 연타해도 로드는 1회만 돈다. */
	TMap<FGameplayTag, TArray<FPendingPopupRequest>> PendingRequests;
	TMap<FGameplayTag,TArray<TWeakObjectPtr<ULastFPSModalDialogBase>>> ActivePopups;
	TWeakObjectPtr<ALastFPSGameStateBase> ContextSource;
	FDelegateHandle PreLoadMapDelegateHandle;
	FDelegateHandle TravelFailureDelegateHandle;
	bool bHasAppliedDestinationContext = false;
	bool bLevelTransitionInProgress = false;
};

/**
 * 본문을 헤더에 두는 이유 — 호출하는 .cpp 에서 정의가 보여야 그 타입의 코드가 만들어진다.
 * .cpp 에 두면 팝업 타입마다 명시적 인스턴스화를 등록해야 하고, 새 팝업을 아무 데서나
 * 열 수 없게 된다. 무거운 처리는 PushPopupInternal 이 맡고 있어 여기 남는 건 형변환뿐이다.
 */
template <typename TPopupWidget>
TPopupWidget* ULastFPSPopupSubsystem::ShowPopup(
	const UObject* WorldContext, const FGameplayTag& PopupTag)
{
	// 팝업이 아닌 위젯을 넘기면 태그를 찾기도 전에 컴파일 단계에서 걸린다.
	static_assert(
		TIsDerivedFrom<TPopupWidget, ULastFPSModalDialogBase>::IsDerived,
		"ShowPopup 은 ULastFPSModalDialogBase 파생 위젯만 열 수 있습니다.");

	// 빈 태그는 서브시스템을 찾을 필요도 없다. 실패 로그는 PushPopupInternal 이 남긴다.
	if (!PopupTag.IsValid())
	{
		return nullptr;
	}

	// static 이라 this 가 없다. 이 팝업이 누구 것인지는 WorldContext 로만 정할 수 있다.
	ULastFPSPopupSubsystem* Subsystem = Get(WorldContext);
	if (!Subsystem)
	{
		return nullptr;
	}

	// static 이지만 같은 클래스의 멤버라 private 인 PushPopupInternal 에 접근할 수 있다.
	TPopupWidget* PopupWidget =
		Cast<TPopupWidget>(Subsystem->PushPopupInternal(PopupTag, TPopupWidget::StaticClass()));
	return PopupWidget;
}

template <typename TPopupWidget>
void ULastFPSPopupSubsystem::ShowPopupAsync(
	const UObject* WorldContext,
	const FGameplayTag& PopupTag,
	TFunction<void(TPopupWidget*)> OnReady)
{
	static_assert(
		TIsDerivedFrom<TPopupWidget, ULastFPSModalDialogBase>::IsDerived,
		"ShowPopupAsync 는 ULastFPSModalDialogBase 파생 위젯만 열 수 있습니다.");

	ULastFPSPopupSubsystem* Subsystem = PopupTag.IsValid() ? Get(WorldContext) : nullptr;
	if (!Subsystem)
	{
		// 시작조차 못 했음을 호출부가 알 수 있도록 실패도 통보한다.
		if (OnReady)
		{
			OnReady(nullptr);
		}
		return;
	}

	// 기반 타입으로 오는 결과를 요청한 타입으로 좁혀 전달한다.
	Subsystem->RequestPopupInternal(
		PopupTag,
		TPopupWidget::StaticClass(),
		[OnReady = MoveTemp(OnReady)](ULastFPSModalDialogBase* Popup)
		{
			if (OnReady)
			{
				OnReady(Cast<TPopupWidget>(Popup));
			}
		});
}
