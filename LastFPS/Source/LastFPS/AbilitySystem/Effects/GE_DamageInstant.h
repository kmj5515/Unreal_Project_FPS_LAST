#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DamageInstant.generated.h"

UCLASS()
class LASTFPS_API ULastFPSGE_DamageInstant : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_DamageInstant();

    virtual void PostLoad() override;

private:
    void ConfigureDamageModifier();
};
