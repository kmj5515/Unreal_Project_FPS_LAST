#pragma once

#include "CoreMinimal.h"
#include "LastFPSEnumTypes.generated.h"

UENUM(BlueprintType)
enum class EMMLocomotionState : uint8
{
    Idle    UMETA(DisplayName = "Idle"),
    Walk    UMETA(DisplayName = "Walk"),
    Jog     UMETA(DisplayName = "Jog"),
    Sprint  UMETA(DisplayName = "Sprint"),
};

UENUM(BlueprintType)
enum class EMMStance : uint8
{
    Standing  UMETA(DisplayName = "Standing"),
    Crouching UMETA(DisplayName = "Crouching"),
};

UENUM(BlueprintType)
enum class EMMAimMode : uint8
{
    Hip UMETA(DisplayName = "Hip"),
    ADS UMETA(DisplayName = "ADS"),
};

UENUM(BlueprintType)
enum class EMMCombatState : uint8
{
    Idle    UMETA(DisplayName = "Idle"),
    Attacking   UMETA(DisplayName = "Attacking"),
    Reloading   UMETA(DisplayName = "Reloading"),
    Dashing     UMETA(DisplayName = "Dashing"),
    
};


UENUM(BlueprintType)
enum class EMMAirState : uint8
{
    Grounded UMETA(DisplayName = "Grounded"),
    Jumping  UMETA(DisplayName = "Jumping"),
    Falling  UMETA(DisplayName = "Falling"),
};

UENUM(BlueprintType)
enum class EMMWeaponType : uint8
{
    Unarmed UMETA(DisplayName = "Unarmed"),
    Rifle   UMETA(DisplayName = "Rifle"),
    Pistol  UMETA(DisplayName = "Pistol"),
};

// ------------------------------------- Input ID 
// UENUM(BlueprintType)
// enum class EAbilityInputID :uint8
// {
//     None					UMETA(DisplayName ="None"),
//     BasicAttack				UMETA(DisplayName ="BasicAttack"),
//     Aim						UMETA(DisplayName ="Aim"),
//     Dodge					UMETA(DisplayName ="Dodge"),
//     AbilityOne				UMETA(DisplayName ="AbilityOne"),
//     AbilityTwo				UMETA(DisplayName ="AbilityTwo"),
//     AbilityThree			UMETA(DisplayName ="AbilityThree"),
//     AbilityFour				UMETA(DisplayName ="AbilityFour"),
//     AbilityFive				UMETA(DisplayName ="AbilityFive"),
//     AbilitySix				UMETA(DisplayName ="AbilitySix"),
// };
