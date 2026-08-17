#include "Utility/LastFPSPawnTeleport.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace LastFPSPawnTeleport
{
	bool TeleportPawnTo(APawn& Pawn, const FTransform& Destination)
	{
		const FRotator DestinationRotation = Destination.Rotator();

		// 파티원이 같은 한 점으로 몰리므로 서로 파묻히지 않게 빈 자리를 먼저 찾는다.
		FVector TargetLocation = Destination.GetLocation();
		if (UWorld* World = Pawn.GetWorld())
		{
			World->FindTeleportSpot(&Pawn, TargetLocation, DestinationRotation);
		}

		// bIsATest=false, bNoCheck=true — 빈 자리를 이미 찾았으므로 재검사하지 않는다.
		if (!Pawn.TeleportTo(TargetLocation, DestinationRotation, false, true))
		{
			return false;
		}

		if (ACharacter* Character = Cast<ACharacter>(&Pawn))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				// 이동 전 속도를 남기면 도착 직후 관성으로 밀려난다.
				Movement->StopMovementImmediately();
			}
		}

		// 컨트롤 회전을 맞추지 않으면 다음 입력에서 시야가 원래 방향으로 튄다.
		if (AController* Controller = Pawn.GetController())
		{
			Controller->SetControlRotation(DestinationRotation);
		}

		return true;
	}
}
