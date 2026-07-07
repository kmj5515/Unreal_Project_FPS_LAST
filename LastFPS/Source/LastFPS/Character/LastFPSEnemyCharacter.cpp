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

    AttributeSet->SetMaxHealth(MaxHealth);
    AttributeSet->SetHealth(MaxHealth);
}
