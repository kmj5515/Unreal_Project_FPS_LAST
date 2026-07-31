#pragma once

#include "UI/Framework/LastFPSButtonBase.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSTravelEntryButton.generated.h"

/**
 * 이동 목적지만 보관하고 실제 이동 수행은 소유 화면과 이동 서브시스템에 위임한다.
 */
UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSTravelEntryButton : public ULastFPSButtonBase
{
	GENERATED_BODY()

public:
	const FLastFPSTravelEntryRequest& GetTravelRequest() const
	{
		return TravelRequest;
	}

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Travel",
		meta=(AllowPrivateAccess="true"))
	FLastFPSTravelEntryRequest TravelRequest;
};
