#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSItemPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 월드에 떨어지는 아이템 드랍. Hero 가 밟으면 그 플레이어 PlayerState 로 지급을 위임하고 파괴된다.
 * (PlayerState 가 소유 클라의 로컬 Economy 에 넣어주므로 리슨서버 원격 클라도 올바른 대상에 지급.)
 */
UCLASS()
class LASTFPS_API ALastFPSItemPickupActor : public AActor
{
    GENERATED_BODY()

public:
    ALastFPSItemPickupActor();

    // 지급할 아이템 행 ID / 개수 (스폰 시 주입).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ExposeOnSpawn="true"))
    FName ItemRowId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ExposeOnSpawn="true"))
    int32 Count = 1;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<USphereComponent> OverlapSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UStaticMeshComponent> PickupMesh;

    UPROPERTY(EditDefaultsOnly, Category="Pickup")
    float PickupRadius = 100.f;

private:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);

    void TryGrant(AActor* OtherActor);
};
