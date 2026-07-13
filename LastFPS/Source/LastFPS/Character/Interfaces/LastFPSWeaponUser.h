#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSWeaponUser.generated.h"

class UWeaponComponent;

UINTERFACE(MinimalAPI)
class ULastFPSWeaponUser : public UInterface
{
	GENERATED_BODY()
};

/** 무기를 사용하는 캐릭터가 제공해야 하는 최소 계약이다. */
class LASTFPS_API ILastFPSWeaponUser
{
	GENERATED_BODY()

public:
	virtual UWeaponComponent* GetWeaponComponent() const = 0;
};
