#include "Animation/LastFPSAnimInstance.h"
#include "Character/LastFPSCharacterBase.h"
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
    }
}

void ULastFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }

    FVector Velocity = MovementComponent->Velocity;
    Velocity.Z = 0.f;
    Speed = Velocity.Size();

    if (Speed > 0.f)
    {
        FRotator ActorRotation = OwnerCharacter->GetActorRotation();
        FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
        Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
    }
    else
    {
        Direction = 0.f;
    }

    bIsInAir = MovementComponent->IsFalling();

    if (ALastFPSCharacterBase* Base = Cast<ALastFPSCharacterBase>(OwnerCharacter))
    {
        bIsADS = Base->GetIsADS();
        bIsDead = !Base->IsAlive();
    }
}
