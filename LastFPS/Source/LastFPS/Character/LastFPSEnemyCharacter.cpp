#include "Character/LastFPSEnemyCharacter.h"

#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"

ALastFPSEnemyCharacter::ALastFPSEnemyCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ALastFPSEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!HasAuthority() || !AttributeSet)
    {
        return;
    }

    const float ClampedMax = FMath::Max(1.f, MaxHealthOverride);
    AttributeSet->SetMaxHealth(ClampedMax);
    AttributeSet->SetHealth(ClampedMax);
}
