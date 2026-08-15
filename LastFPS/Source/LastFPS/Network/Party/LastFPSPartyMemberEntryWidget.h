#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LastFPSPartyMemberEntryWidget.generated.h"

class UTextBlock;
class UImage;
class ULastFPSPartyMemberData;

/**
 * 
 */
UCLASS()
class LASTFPS_API ULastFPSPartyMemberEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	void RefreshData(class ULastFPSPartyMemberData* Data);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	// In blueprint, bind to "PlayerNameText"
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PlayerNameText;

	// In blueprint, bind to "ReadyStatusText"
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ReadyStatusText;

	// In blueprint, bind to "HostIcon"
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HostIcon;

	UFUNCTION(BlueprintImplementableEvent, Category = "Party")
	void BP_OnDataUpdated(ULastFPSPartyMemberData* MemberData);
};
