#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Skill2Cooldown.generated.h"

/** E 스킬 쿨다운 (CommitAbility용) */
UCLASS()
class LASTFPS_API ULastFPSGE_Skill2Cooldown : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_Skill2Cooldown();
};
