#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Perception/AIPerceptionTypes.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "LastFPSEnemyCharacter.generated.h"

class ULastFPSAIProfile;
class ULastFPSCombatAimComponent;
class UWeaponComponent;
class UWidgetComponent;

UCLASS()
class LASTFPS_API ALastFPSEnemyCharacter
    : public ALastFPSCharacterBase
    , public ILastFPSWeaponUser
    , public ILastFPSPoolableActor
{
    GENERATED_BODY()

public:
    ALastFPSEnemyCharacter();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 웨이브에 마지막으로 남은 적임을 알리는 머리 위 마커를 켜거나 끈다. 서버에서 호출한다. */
    void SetWaveEnemyMarkerVisible(bool bVisible);

    virtual UWeaponComponent* GetWeaponComponent() const override { return WeaponComponent; }

    UFUNCTION(BlueprintPure, Category="Enemy|Combat Aim")
    ULastFPSCombatAimComponent* GetCombatAimComponent() const { return CombatAimComponent; }

    /**
     * 이 적의 AI 행동 프로파일. 해석된 CharacterDefinition 이 ULastFPSEnemyDefinition 일 때
     * 그 AIProfile 을 반환한다. 없으면 nullptr.
     * 컨트롤러/BT 노드가 감지·공격 파라미터를 읽는 단일 진입점.
     */
    UFUNCTION(BlueprintPure, Category="Enemy|AI")
    const ULastFPSAIProfile* GetAIProfile() const;

    virtual void ResetForPoolReuse(ULastFPSCharacterDefinition* InDefinition) override;
    virtual void OnAcquiredFromPool_Implementation() override;
    virtual void OnReleasedToPool_Implementation() override;
    virtual void OnPrepareForPoolRenderWarmup_Implementation() override;

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UpdateAliveCollisionState(bool bAlive) override;
    
    UPROPERTY(EditAnywhere, Category="Enemy|AI")
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Weapon")
    TObjectPtr<UWeaponComponent> WeaponComponent;

    /** GA가 생산한 조준 상태를 AnimBP와 원격 클라이언트에 전달한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Combat Aim")
    TObjectPtr<ULastFPSCombatAimComponent> CombatAimComponent;

    /** 마지막 남은 적을 표시하는 화면 공간 화살표다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Wave Marker")
    TObjectPtr<UWidgetComponent> WaveEnemyMarkerComponent;

    /** 캡슐 상단에서 화살표까지의 추가 높이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Wave Marker", meta=(ClampMin="0.0", Units="cm"))
    float WaveEnemyMarkerVerticalOffset = 45.f;

    // 사망 시 드랍할 픽업 (비우면 드랍 없음).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TSubclassOf<ALastFPSItemPickupActor> DropPickupClass;

    // 확정 드랍. 여기 나열한 RowId 는 항상 1개씩 나온다(같은 걸 여러 번 나열하면 그 수만큼). 총량(DropCount)에 포함.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TArray<FName> GuaranteedDropRowIds;

    // 확정분을 채우고 남은 개수를 채울 후보(가중치 랜덤). 비면 랜덤분 없음.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    TArray<FLastFPSEnemyDropEntry> DropTable;

    // 총 드랍 픽업 개수(확정 + 랜덤). 확정분이 이보다 많으면 확정분은 모두 나온다. 각 픽업은 1개씩 지급.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    int32 DropCount = 1;

    // 드랍을 뿌릴 원 반경 (cm).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Drop")
    float DropSpreadRadius = 150.f;

    /** 사망 후 랙돌이 월드에 남아 있는 시간이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Death", meta=(ClampMin="0.1", Units="s"))
    float DeathRemovalDelay = 5.f;

    /** 사망 시 마지막 공격자로부터 멀어지는 방향으로 적용할 속도 변화량이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Death|Ragdoll", meta=(ClampMin="0.0", Units="cm/s"))
    float DeathRagdollImpulseVelocity = 400.f;

    /** 사망 Impulse에 추가할 위쪽 속도 변화량이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Death|Ragdoll", meta=(ClampMin="0.0", Units="cm/s"))
    float DeathRagdollUpwardVelocity = 150.f;

    /** None이면 전체 Physics Body에 적용하고, 지정하면 해당 Bone과 하위 Body에 적용한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Death|Ragdoll")
    FName DeathRagdollImpulseBoneName = NAME_None;

private:
    UFUNCTION()
    void OnRep_WaveEnemyMarkerVisible();

    void ApplyWaveEnemyMarkerVisibility();

    void ApplyAIControllerClassFromProfile();
    void HandleOwnDeath(ALastFPSCharacterBase* DeadChar);
    void StartDeathRagdoll();
    void ResetDeathRagdoll();
    void FinishDeathRemoval();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ApplyDeathRagdollImpulse(FVector_NetQuantizeNormal ImpulseDirection);

    bool bDeathRagdollStarted = false;
    bool bMeshRelativeTransformCaptured = false;
    FTransform InitialMeshRelativeTransform = FTransform::Identity;
    FTimerHandle DeathRemovalTimerHandle;

    UPROPERTY(ReplicatedUsing=OnRep_WaveEnemyMarkerVisible)
    bool bWaveEnemyMarkerVisible = false;
};
