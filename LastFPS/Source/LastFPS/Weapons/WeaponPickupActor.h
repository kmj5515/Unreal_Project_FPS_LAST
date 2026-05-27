#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/LastFPSEnumTypes.h"
#include "WeaponPickupActor.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class ALastFPSWeaponActor;

UCLASS()
class LASTFPS_API AWeaponPickupActor : public AActor
{
    GENERATED_BODY()

public:
    AWeaponPickupActor();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<USphereComponent> OverlapSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<USkeletalMeshComponent> PickupMesh;

public:
    // 플레이어에게 줄 무기 데이터
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TObjectPtr<USkeletalMesh> WeaponSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSWeaponActor> WeaponActorClass;

protected:

    // 픽업 가능 반경 (cm)
    UPROPERTY(EditDefaultsOnly, Category="Pickup")
    float PickupRadius = 100.f;

private:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);

    void TryEquipToActor(AActor* OtherActor);
};
