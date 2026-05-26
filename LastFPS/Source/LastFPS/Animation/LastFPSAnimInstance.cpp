#include "Animation/LastFPSAnimInstance.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSHero.h"
#include "Character/Components/WeaponComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void ULastFPSAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    OwnerCharacter = Cast<ACharacter>(GetOwningActor());
    if (OwnerCharacter)
    {
        MovementComponent = OwnerCharacter->GetCharacterMovement();

        if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
        {
            if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
            {
                WeaponType = Weapon->GetWeaponType();
                Weapon->OnWeaponEquippedChanged.AddDynamic(this, &ULastFPSAnimInstance::OnWeaponEquipped);
            }
        }
    }
}

void ULastFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }

    UpdateLocomotionState();
    UpdateAirState();
    UpdateStance();
    UpdateCombatState();
    UpdatePivot();
}

void ULastFPSAnimInstance::UpdateLocomotionState()
{
    Velocity = MovementComponent->Velocity;
    Speed = Velocity.Size2D();

    if (Speed > 1.f)
    {
        FRotator ActorRotation = OwnerCharacter->GetActorRotation();
        FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
        Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
    }
    else
    {
        Direction = 0.f;    
    }

    if (Speed < 500.f)
    {
        LocomotionState = EMMLocomotionState::Jog;
    }
    else
    {
        LocomotionState = EMMLocomotionState::Sprint;
    }
}

void ULastFPSAnimInstance::UpdateAirState()
{
    if (MovementComponent->IsFalling())
    {
        AirState = (MovementComponent->Velocity.Z > 0.f)
            ? EMMAirState::Jumping
            : EMMAirState::Falling;
    }
    else
    {
        AirState = EMMAirState::Grounded;
    }
}

void ULastFPSAnimInstance::UpdateStance()
{
    Stance = MovementComponent->IsCrouching()
        ? EMMStance::Crouching
        : EMMStance::Standing;
}

void ULastFPSAnimInstance::UpdateCombatState()
{
    ALastFPSCharacterBase* Base = Cast<ALastFPSCharacterBase>(OwnerCharacter);
    if (!Base)
    {
        return;
    }

    bIsDead = !Base->IsAlive();
    CombatState = Base->GetIsADS() ? EMMCombatState::ADS : EMMCombatState::Hip;

    // Aim Pitch / Yaw: 컨트롤러 회전과 캐릭터 회전의 차이를 -180~180으로 정규화
    if (AController* Controller = OwnerCharacter->GetController())
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator ActorRot   = OwnerCharacter->GetActorRotation();
        const FRotator Delta      = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
        AimPitch = Delta.Pitch;
        AimYaw   = Delta.Yaw;
    }

    // // 발사 중 여부
    // if (ALastFPSHero* Hero = Cast<ALastFPSHero>(Base))
    // {
    //     if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
    //     {
    //         bIsFiring = Weapon->IsFiring();
    //     }
    // }
}

void ULastFPSAnimInstance::OnWeaponEquipped(bool bEquipped)
{
    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerCharacter))
    {
        if (UWeaponComponent* Weapon = Hero->GetWeaponComponent())
        {
            WeaponType = Weapon->GetWeaponType();
            return;
        }
    }
    WeaponType = EMMWeaponType::Unarmed;
}

void ULastFPSAnimInstance::UpdatePivot()
{
    const FVector Acc = MovementComponent->GetCurrentAcceleration();
    if (Velocity.SizeSquared2D() < PivotMinSpeed * PivotMinSpeed
        || Acc.SizeSquared2D() < KINDA_SMALL_NUMBER)
    {
        bIsPivoting = false;
        return;
    }

    const FVector VelDir = Velocity.GetSafeNormal2D();
    const FVector AccDir = Acc.GetSafeNormal2D();
    bIsPivoting = FVector::DotProduct(VelDir, AccDir) < PivotDotThreshold;
}
