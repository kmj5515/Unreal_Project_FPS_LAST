#include "Animation/Notifies/AnimNotify_WeaponMagazineVisual.h"

#include "Character/Components/WeaponComponent.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_WeaponMagazineVisual::Notify(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    AActor* MeshOwner = MeshComp ? MeshComp->GetOwner() : nullptr;
    const ILastFPSWeaponUser* WeaponUser = Cast<ILastFPSWeaponUser>(MeshOwner);
    UWeaponComponent* WeaponComponent = WeaponUser ? WeaponUser->GetWeaponComponent() : nullptr;
    if (!WeaponComponent)
    {
        return;
    }

    switch (Action)
    {
    case ELastFPSWeaponMagazineVisualAction::DetachToHand:
        WeaponComponent->DetachMagazineToHand();
        break;

    case ELastFPSWeaponMagazineVisualAction::RestoreToWeapon:
        WeaponComponent->RestoreMagazineToWeapon();
        break;

    default:
        ensureMsgf(false, TEXT("지원하지 않는 탄창 시각 Notify 동작입니다."));
        break;
    }
}

FString UAnimNotify_WeaponMagazineVisual::GetNotifyName_Implementation() const
{
    switch (Action)
    {
    case ELastFPSWeaponMagazineVisualAction::DetachToHand:
        return TEXT("Magazine: 손으로 분리");

    case ELastFPSWeaponMagazineVisualAction::RestoreToWeapon:
        return TEXT("Magazine: 무기로 복구");

    default:
        return Super::GetNotifyName_Implementation();
    }
}
