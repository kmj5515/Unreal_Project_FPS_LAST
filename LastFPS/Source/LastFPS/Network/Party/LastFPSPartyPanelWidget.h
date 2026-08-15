#pragma once

#include "CoreMinimal.h"
#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSPartyPanelWidget.generated.h"

class UListView;
class UButton;
class UTextBlock;
class ALastFPSPartyGameState;
class ALastFPSPartyPlayerState;

/**
 * 
 */
UCLASS()
class LASTFPS_API ULastFPSPartyPanelWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()
	
public:
	ULastFPSPartyPanelWidget(const FObjectInitializer& ObjectInitializer);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UListView* PartyMemberList;

	UPROPERTY(meta = (BindWidget))
	UButton* ApplyButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ApplyText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayerCountText;

	UFUNCTION()
	void OnApplyButtonClicked();

	UFUNCTION()
	void RefreshPartyList();

	UFUNCTION()
	void OnPartyStateChanged(class ALastFPSPartyPlayerState* PS);

	TWeakObjectPtr<ALastFPSPartyGameState> CachedGameState;
	FTimerHandle PollingTimerHandle;

	void TryBindToGameState();
};
