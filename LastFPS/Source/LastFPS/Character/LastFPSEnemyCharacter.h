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

UCLASS()
class LASTFPS_API ALastFPSEnemyCharacter
    : public ALastFPSCharacterBase
    , public ILastFPSWeaponUser
    , public ILastFPSPoolableActor
{
    GENERATED_BODY()

public:
    ALastFPSEnemyCharacter();

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
};
