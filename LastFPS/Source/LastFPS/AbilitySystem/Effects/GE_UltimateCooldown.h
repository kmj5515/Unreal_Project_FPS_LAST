#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_UltimateCooldown.generated.h"

/** 궁극기(F) 쿨다운 (CommitAbility용) — 다른 스킬(Q/E)과 동일 패턴 */
UCLASS()
class LASTFPS_API ULastFPSGE_UltimateCooldown : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_UltimateCooldown();
};
