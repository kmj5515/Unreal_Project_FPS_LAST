#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LastFPSPartyGameState.generated.h"

class ALastFPSPartyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyMembersUpdated);

/**
 * 
 */
UCLASS()
class LASTFPS_API ALastFPSPartyGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ALastFPSPartyGameState();

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintCallable, Category = "Party")
	TArray<ALastFPSPartyPlayerState*> GetPartyMembers() const;

	UPROPERTY(BlueprintAssignable, Category = "Party")
	FOnPartyMembersUpdated OnPartyMembersUpdated;
};
