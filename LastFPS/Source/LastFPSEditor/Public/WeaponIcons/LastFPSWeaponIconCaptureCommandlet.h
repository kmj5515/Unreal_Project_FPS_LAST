#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LastFPSWeaponIconCaptureCommandlet.generated.h"

/** 프로젝트의 무기 슬롯 아이콘과 PNG를 사용자 UI 없이 일괄 생성한다. */
UCLASS()
class LASTFPSEDITOR_API ULastFPSWeaponIconCaptureCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	ULastFPSWeaponIconCaptureCommandlet();

	virtual int32 Main(const FString& Params) override;
};
