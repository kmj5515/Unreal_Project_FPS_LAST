#pragma once

#include "GameUIPolicy.h"
#include "LastFPSUIPolicy.generated.h"

/** Default UI policy — LayoutClass points to ULastFPSPrimaryGameLayout (or a WBP child) */
UCLASS()
class LASTFPS_API ULastFPSUIPolicy : public UGameUIPolicy
{
	GENERATED_BODY()

public:
	ULastFPSUIPolicy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostInitProperties() override;
};
