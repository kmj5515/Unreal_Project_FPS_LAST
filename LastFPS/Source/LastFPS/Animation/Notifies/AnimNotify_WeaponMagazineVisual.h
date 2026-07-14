#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WeaponMagazineVisual.generated.h"

UENUM(BlueprintType)
enum class ELastFPSWeaponMagazineVisualAction : uint8
{
    DetachToHand UMETA(DisplayName="손으로 탄창 분리"),
    RestoreToWeapon UMETA(DisplayName="무기로 탄창 복구")
};

UCLASS(meta=(DisplayName="Weapon Magazine Visual"))
class LASTFPS_API UAnimNotify_WeaponMagazineVisual : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;

private:
    UPROPERTY(EditAnywhere, Category="Magazine")
    ELastFPSWeaponMagazineVisualAction Action = ELastFPSWeaponMagazineVisualAction::DetachToHand;
};
