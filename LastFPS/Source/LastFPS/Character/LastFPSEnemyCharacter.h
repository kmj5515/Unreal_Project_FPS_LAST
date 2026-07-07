#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "LastFPSEnemyCharacter.generated.h"

UCLASS()
class LASTFPS_API ALastFPSEnemyCharacter : public ALastFPSCharacterBase
{
    GENERATED_BODY()

public:
    ALastFPSEnemyCharacter();

protected:
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Stats")
    float MaxHealth = 10000.f;
};
