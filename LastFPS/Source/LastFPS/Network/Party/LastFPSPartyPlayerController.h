#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastFPSPartyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class LASTFPS_API ALastFPSPartyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALastFPSPartyPlayerController();
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Party")
	void ServerSetPartyReady(bool bIsReady);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Party")
	void ServerRequestStartPartyGame();

protected:
	virtual void BeginPlay() override;
	virtual void ReceivedPlayer() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<class UCommonActivatableWidget> PartyPanelWidgetClass;
};
