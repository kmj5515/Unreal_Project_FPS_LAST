#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_UltimateKillHeal.generated.h"

/** 궁극기 발동 후 킬 시 적용하는 즉시 회복 */
UCLASS()
class LASTFPS_API ULastFPSGE_UltimateKillHeal : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_UltimateKillHeal();
};
