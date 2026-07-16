#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

namespace LastFPSCollision
{
    /** 프로젝트 설정의 Pickup Object Channel과 반드시 같은 슬롯을 사용해야 한다. */
    inline constexpr ECollisionChannel PickupObjectChannel = ECC_GameTraceChannel1;

    inline const FName PickupProfileName = TEXT("Pickup");

    /** 획득 Trigger는 Pawn과만 겹치고 무기 Trace 및 Projectile과는 충돌하지 않는다. */
    inline void ConfigurePickupTriggerCollision(UPrimitiveComponent& TriggerComponent)
    {
        TriggerComponent.SetCollisionProfileName(PickupProfileName);
        TriggerComponent.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        TriggerComponent.SetCollisionObjectType(PickupObjectChannel);
        TriggerComponent.SetCollisionResponseToAllChannels(ECR_Ignore);
        TriggerComponent.SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        TriggerComponent.SetGenerateOverlapEvents(true);
    }
}
