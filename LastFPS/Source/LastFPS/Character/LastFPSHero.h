#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSHero.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class ULastFPSInputConfig;
class UWeaponComponent;
class UAnimMontage;
class UCameraShakeBase;
struct FInputActionValue;

UCLASS()
class LASTFPS_API ALastFPSHero : public ALastFPSCharacterBase
{
    GENERATED_BODY()

public:
    ALastFPSHero();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayWeaponFireEffects();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
    virtual bool GetIsADS() const override { return bIsADS; }
    FORCEINLINE bool GetIsSprinting() const { return bIsSprinting; }
    FORCEINLINE bool GetWantsToSprint() const { return bWantsToSprint; }
    FORCEINLINE bool GetWantsToWalk() const { return bWantsToWalk; }
    FORCEINLINE EMMCombatState GetCombatState() const { return CombatState; }
    FORCEINLINE FVector2D GetCachedMoveInput() const { return CachedMoveInput; }
    FORCEINLINE FRotator GetLocomotionDirectionBaseRotation() const
    {
        return bHasMoveInputAction ? LocomotionDirectionBaseRotation : GetActorRotation();
    }
    FORCEINLINE bool HasMoveInputAction() const { return bHasMoveInputAction; }
    FORCEINLINE int32 GetJumpStartSequence() const { return JumpStartSequence; }
    bool HasForwardSprintInput() const;
    bool CanStartSprint() const;

    // GAS 어빌리티에서 카메라 줌을 제어할 수 있도록 공개
    void SetADS(bool bEnabled);
    void SetSprinting(bool bEnabled);
    void SetWantsToSprint(bool bEnabled);
    void SetWantsToWalk(bool bEnabled);
    void SetCombatState(EMMCombatState NewState);
    void NotifyJumpStarted();

    /**
     * 게임플레이 입력(이동/사격/궁극기/ADS/스프린트 등)을 통째로 켜고 끈다.
     * DefaultMappingContext를 복원/제거 → Enhanced Input 액션 전체가 활성/정지된다.
     * NPC 상호작용·컷씬 등에서 소스 레벨로 입력을 차단할 때 사용.
     * (ESC·G 등 PlayerController가 BindKey로 건 입력은 별개라 영향 없음)
     */
    void SetGameplayInputEnabled(bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GiveDefaultAbilities() override;
    virtual void OnCombatEngagedChanged() override;

    void Move(const FInputActionValue& Value);
    void ClearMoveInput(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    // 공통 GAS 입력 처리
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag);
    bool TryConfirmActiveAbility(FGameplayTag InputID);
    bool TryCancelActiveAbility(FGameplayTag InputID);
    bool ShouldBlockAbilityInputRelease(FGameplayTag InputID);
    void CancelAbilityByTag(FGameplayTag AbilityTag);
    void InputPressed(FGameplayTag InputID);
    void InputReleased(FGameplayTag InputID);

    void TickCameraInterp(float DeltaTime);
    void ApplyRotationModeSettings();
    bool ShouldUseControllerYawRotationMode() const;
    bool ShouldOrientRotationToMovement() const;
    bool ShouldUseWeaponStrafeRotationMode() const;
    bool ShouldFaceCameraForwardFromMoveInput(const FVector2D& MovementVector) const;
    FRotator FaceCameraYaw(float DeltaTime);
    bool HasEquippedWeapon() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    float DefaultArmLength = 300.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    FVector DefaultSocketOffset = FVector(0.f, 60.f, 20.f);

    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    float DefaultFOV = 90.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSArmLength = 90.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    FVector ADSSocketOffset = FVector(0.f, 25.f, 35.f);

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSFOV = 65.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSInterpSpeed = 18.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSWalkSpeed = 300.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float CameraLagSpeed = 15.f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint")
    float SprintForwardInputThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Walk")
    float WalkMaxWalkSpeed = 250.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UWeaponComponent> WeaponComponent;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<ULastFPSInputConfig> InputConfig;

private:
    bool bIsADS = false;
    float PreADSWalkSpeed = 0.f;
    float PreWalkMaxWalkSpeed = 0.f;
    FVector2D CachedMoveInput = FVector2D::ZeroVector;
    FRotator LocomotionDirectionBaseRotation = FRotator::ZeroRotator;
    bool bHasMoveInputAction = false;

    UPROPERTY(ReplicatedUsing=OnRep_CombatState, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true"))
    EMMCombatState CombatState = EMMCombatState::Idle;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    int32 JumpStartSequence = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    bool bWantsToSprint = false;

    UPROPERTY(BlueprintReadOnly, Category="Movement", meta=(AllowPrivateAccess="true"))
    bool bWantsToWalk = false;

    float TargetArmLength;
    FVector TargetSocketOffset;
    float TargetFOV;

    UFUNCTION()
    void OnRep_CombatState();

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    void HandleAbilityInput(const FInputActionValue& value, FGameplayTag InputID);
    bool ShouldCancelFireBeforeAbilityInput(FGameplayTag InputID) const;
    bool ShouldSkipAbilityCancelOnRelease(FGameplayTag InputID) const;
    void RestoreWalkSpeed();

    void TickLocalMatchIntro();

    bool bLocalMatchIntroInputDisabled = false;
    bool bLocalMatchIntroFxStarted = false;
};
