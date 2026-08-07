#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "Templates/SubclassOf.h"
#include "LastFPSItemPickupActor.generated.h"

class ALastFPSHero;
class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class UNiagaraComponent;
class UGameplayEffect;
class UMaterialInstanceDynamic;

/**
 * 월드에 떨어지는 드랍 픽업. Hero가 밟으면 지정된 지급 수단을 적용하고 풀로 반환한다.
 * 등록된 풀이 없거나 소진된 경우에만 기존처럼 파괴한다.
 * 아이템·체력·탄약 픽업이 이 클래스를 공유하고, 무엇을 지급할지는 픽업 BP 의 값이 정한다.
 */
UCLASS()
class LASTFPS_API ALastFPSItemPickupActor
    : public AActor
    , public ILastFPSPoolableActor
{
    GENERATED_BODY()

public:
    ALastFPSItemPickupActor();

    // 지급할 아이템 행 ID / 개수 (스폰 시 주입). RowId 는 클라의 등급 발광색 계산을 위해 복제된다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ItemRowId, Category="Pickup", meta=(ExposeOnSpawn="true"))
    FName ItemRowId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ExposeOnSpawn="true"))
    int32 Count = 1;

    // 착지 지점(=액터 위치) 기준, 발사 연출을 시작할 상대 오프셋(적 중심 방향). 0이면 연출 없이 즉시 착지.
    UPROPERTY(ReplicatedUsing=OnRep_LaunchOffset)
    FVector LaunchStartOffset = FVector::ZeroVector;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

    void InitializePickup(FName InItemRowId, int32 InCount, FVector InLaunchStartOffset);

    virtual void OnAcquiredFromPool_Implementation() override;
    virtual void OnReleasedToPool_Implementation() override;
    virtual void OnPrepareForPoolRenderWarmup_Implementation() override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<USphereComponent> OverlapSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UStaticMeshComponent> PickupMesh;

    // 메시만 로컬 회전(시각용). 트리거 스피어는 고정.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<URotatingMovementComponent> RotatingMovement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UNiagaraComponent> SpawnVFX;

    /** 밟았을 때 적용할 즉시 회복 GE. 비우면 회복 없음. 회복량은 이 GE 가 소유한다. */
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Grant")
    TSubclassOf<UGameplayEffect> HealEffectClass;

    /** 밟았을 때 보급할 예비 탄약. 0 이면 보급 없음. */
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Grant", meta=(ClampMin="0"))
    int32 ReserveAmmoAmount = 0;

    UPROPERTY(EditDefaultsOnly, Category="Pickup")
    float PickupRadius = 100.f;

    // 초당 회전량(도). 기본은 Yaw 90°/s.
    UPROPERTY(EditDefaultsOnly, Category="Pickup")
    FRotator RotationRate = FRotator(0.f, 90.f, 0.f);

    // 아이템 등급 색을 넣을 머티리얼 벡터 파라미터 이름.
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Rarity")
    FName EmissiveParameterName = TEXT("Emissive_Color");

    // 발광 밝기 배율 (블룸 발광을 위한 HDR 부스트). 등급 색 RGB 에 곱해진다.
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Rarity", meta=(ClampMin="0.0"))
    float EmissiveIntensity = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Pickup")
    FName SpawnFXColorParameterName = TEXT("Color");

    // 발사 연출 지속 시간(초). 0 이하면 연출 생략.
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Launch", meta=(ClampMin="0.0"))
    float LaunchDuration = 0.6f;

    // 포물선 정점 높이(cm).
    UPROPERTY(EditDefaultsOnly, Category="Pickup|Launch", meta=(ClampMin="0.0"))
    float LaunchArcHeight = 150.f;

private:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);

    void TryGrant(AActor* OtherActor);

    // 실제로 지급했을 때만 true.
    bool TryGrantItem(ALastFPSHero& Hero) const;
    bool TryGrantHealth(ALastFPSHero& Hero) const;
    bool TryGrantReserveAmmo(ALastFPSHero& Hero) const;

    // ItemRowId 등급을 조회해 픽업 메시 머티리얼의 발광색에 적용 (서버/클라 공통).
    void ApplyRarityVisual();

    UFUNCTION()
    void OnRep_ItemRowId();

    // 발사 연출 시작(전 인스턴스 로컬). 중복 호출/오프셋 0 이면 무시.
    void TryStartLaunch();

    // 착지 처리(서버). 픽업 가능 상태로 전환하고 즉시 겹친 Hero 처리.
    void HandleLanded();

    void PlaySpawnFX();
    void ActivatePickup();
    void FinishPickup();

    UFUNCTION()
    void OnRep_LaunchOffset();

    ELastFPSItemRarity CachedRarity = ELastFPSItemRarity::Common;
    FLinearColor CachedRarityColor = FLinearColor::White;
    bool bSpawnFXPlayed = false;

    FVector MeshRestRelativeLocation = FVector::ZeroVector;
    float LaunchElapsed = 0.f;
    bool bLaunching = false;
    bool bLaunchResolved = false; // 연출 시작을 이미 처리했는지(중복 방지).
    bool bLanded = false;         // 착지 완료(서버 기준) — 이전엔 못 줍는다.
    bool bMeshRestTransformCaptured = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> CachedMaterialInstances;
};
