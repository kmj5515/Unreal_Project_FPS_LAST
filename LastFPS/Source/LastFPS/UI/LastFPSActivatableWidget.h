#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LastFPSActivatableWidget.generated.h"

/** 프로젝트 공통 Activatable 베이스 (Modal / Menu / HUD) */
UCLASS(Abstract)
class LASTFPS_API ULastFPSActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool NativeOnHandleBackAction() override;
};
