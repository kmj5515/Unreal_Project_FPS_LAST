#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "LastFPSPopupSubsystem.generated.h"

class ALastFPSGameStateBase;
class UClass;
class ULastFPSDialogueWidget;
class ULastFPSModalDialogBase;
class ULastFPSPopupCatalog;
class ULastFPSQuantityDialogWidget;
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

	ULastFPSQuantityDialogWidget* ShowQuantityPrompt(
		const FText& Title,
		const FText& ItemName,
		int32 UnitPrice,
		int32 MaxQuantity);

	ULastFPSDialogueWidget* ShowDialogue(
		const FText& Speaker,
		const TArray<FText>& Lines);

	void BindDestinationContextSource(ALastFPSGameStateBase* GameState);

	void ApplyDestinationContext(
		const FGameplayTagContainer& NewContextTags);

private:
	template<typename TPopupWidget>
	TPopupWidget* PushPopup(const FGameplayTag& PopupTag);

	void LoadCatalog();
	void BuildEntryIndex();
	const FLastFPSPopupEntry* FindEntry(
		const FGameplayTag& PopupTag) const;
	bool DoesEntryMatchContext(
		const FLastFPSPopupEntry& Entry,
		const FGameplayTagContainer& ContextTags) const;
	void BeginLoadEntry(const FLastFPSPopupEntry& Entry);
	void HandleEntryLoadCompleted(FGameplayTag PopupTag);
	UClass* EnsureEntryClassLoaded(const FGameplayTag& PopupTag);
	void ReleaseEntry(const FGameplayTag& PopupTag);
	void CloseActivePopup(const FGameplayTag& PopupTag);
	void HandlePopupDeactivated(
		FGameplayTag PopupTag,
		TWeakObjectPtr<ULastFPSModalDialogBase> Popup);
	void CloseAllActivePopups();
	void CloseEntriesForLevelTransition();
	void ReleaseEntriesForLevelTransition();
	void HandlePreLoadMap(
		const FWorldContext& WorldContext,
		const FString& MapName);
	void HandleDestinationContextChanged(
		const FGameplayTagContainer& NewContextTags);

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSPopupCatalog> PopupCatalog;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UClass>> LoadedWidgetClasses;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveContextTags;

	TMap<FGameplayTag, int32> EntryIndices;
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> LoadHandles;
	TMap<
		FGameplayTag,
		TArray<TWeakObjectPtr<ULastFPSModalDialogBase>>> ActivePopups;
	TWeakObjectPtr<ALastFPSGameStateBase> ContextSource;
	FDelegateHandle PreLoadMapDelegateHandle;
	FDelegateHandle TravelFailureDelegateHandle;
	bool bHasAppliedDestinationContext = false;
	bool bLevelTransitionInProgress = false;
};
