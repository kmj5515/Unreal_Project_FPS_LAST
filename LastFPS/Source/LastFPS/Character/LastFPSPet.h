#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LastFPSPet.generated.h"

class USphereComponent;
class ALastFPSItemPickupActor;

UENUM(BlueprintType)
enum class ELastFPSPetState : uint8
{
    Idle,
    Following,
    Looting
};

/**
 * 아이템을 자동으로 탐색하고 획득하는 펫 캐릭터.
 * 서버에서 이동 및 타겟팅을 처리하며, 획득한 아이템은 Owner인 Hero의 PlayerState로 지급된다.
 * CharacterMovementComponent를 사용하므로 중력이 적용되고 바닥 위를 걸어다닌다.
 */
UCLASS(Blueprintable)
class LASTFPS_API ALastFPSPet : public ACharacter
{
    GENERATED_BODY()

public:
    ALastFPSPet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaTime) override;

    // 실제 주인 설정/조회 (AIController가 Owner를 덮어쓰므로 별도 보관)
    void SetOwnerHero(AActor* InOwner) { OwnerHero = InOwner; }
    AActor* GetOwnerHero() const { return OwnerHero.Get(); }

protected:
    virtual void BeginPlay() override;

    // 아이템 탐색용 넓은 스피어
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pet")
    TObjectPtr<USphereComponent> DetectSphere;

    // 펫의 실제 픽업 충돌 범위
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pet")
    TObjectPtr<USphereComponent> OverlapSphere;

    UPROPERTY(EditDefaultsOnly, Category = "Pet")
    float DetectRadius = 1500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Pet", meta = (ClampMin = "0.1"))
    float TargetScanInterval = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Pet")
    float PickupRadius = 80.f;

    // 주인 곁을 따라갈 때의 목표 오프셋 (X: 뒤쪽, Y: 왼쪽)
    UPROPERTY(EditDefaultsOnly, Category = "Pet")
    FVector FollowOffset = FVector(-50.f, -150.f, 0.f);

    // 주인과 너무 멀어지면 자동으로 워프(순간이동)할 기준 거리
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pet")
    float WarpDistance = 3000.f;

    // 타겟 아이템
    UPROPERTY(Transient)
    TWeakObjectPtr<ALastFPSItemPickupActor> TargetItem;

    // 실제 주인 (AIController가 Owner를 덮어쓰므로 별도 보관)
    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> OwnerHero;

    // 애니메이션 구동용 상태
    UPROPERTY(ReplicatedUsing = OnRep_PetState, BlueprintReadOnly, Category = "Pet")
    ELastFPSPetState PetState = ELastFPSPetState::Idle;

    // 길찾기 연속 호출 방지용
    FVector LastMoveTarget = FVector::ZeroVector;
    float LastMoveRequestTime = 0.f;
    float LastTargetScanTime = -1.f;

private:
    UFUNCTION()
    void OnRep_PetState();
    // 이동 처리
    void UpdateMovement(float DeltaTime);

    // 다음 목표물 탐색
    void FindNextTarget();

    bool IsItemReachable(const ALastFPSItemPickupActor& Item) const;
};
