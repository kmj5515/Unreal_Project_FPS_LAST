#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Skill1Cooldown.generated.h"

/** Q 스킬 쿨다운 (CommitAbility용) */
UCLASS()
class LASTFPS_API ULastFPSGE_Skill1Cooldown : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_Skill1Cooldown();
};
