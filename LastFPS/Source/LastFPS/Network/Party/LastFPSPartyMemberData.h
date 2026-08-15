#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LastFPSPartyMemberData.generated.h"

class ALastFPSPartyPlayerState;

/**
 * Data object for the Party Member ListView
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSPartyMemberData : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "Party")
	TWeakObjectPtr<ALastFPSPartyPlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "Party")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Party")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Party")
	bool bIsHost = false;
};
