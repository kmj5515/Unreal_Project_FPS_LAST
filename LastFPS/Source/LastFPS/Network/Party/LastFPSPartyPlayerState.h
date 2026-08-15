#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LastFPSPartyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyPlayerStateChanged, ALastFPSPartyPlayerState*, PlayerState);

/**
 * 
 */
UCLASS()
class LASTFPS_API ALastFPSPartyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ALastFPSPartyPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	// ~ Begin APlayerState Interface
	virtual void CopyProperties(APlayerState* PlayerState) override;
	// ~ End APlayerState Interface

	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsPartyReady() const { return bIsReady; }

	void SetPartyReady(bool bInReady);

	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsPartyHost() const { return bIsHost; }

	void SetPartyHost(bool bInHost);

	UPROPERTY(BlueprintAssignable, Category = "Party")
	FOnPartyPlayerStateChanged OnPartyStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady;

	UPROPERTY(ReplicatedUsing = OnRep_IsHost)
	bool bIsHost;

	UFUNCTION()
	void OnRep_IsReady();

	UFUNCTION()
	void OnRep_IsHost();
};
