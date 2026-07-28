#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/Interfaces/LastFPSWeaponUser.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSHero.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class ULastFPSInputConfig;
class UWeaponComponent;
class ULastFPSGrapplingAnimationComponent;
class ULastFPSGrapplingTargetingComponent;
class UAnimMontage;
class UUserWidget;
struct FInputActionValue;
struct FLastFPSWeaponScopeSettings;

struct FLastFPSTemporaryCameraEffectOptions
{
    float CameraLagSpeed = 15.f;
    float MotionBlurAmount = 0.f;
    float BlendOutDuration = 0.25f;
};

UCLASS()
class LASTFPS_API ALastFPSHero : public ALastFPSCharacterBase, public ILastFPSWeaponUser
{
    GENERATED_BODY()

public:
    ALastFPSHero();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void PawnClientRestart() override;
    virtual void OnRep_PlayerState() override;

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayWeaponFireEffects();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    virtual UWeaponComponent* GetWeaponComponent() const override { return WeaponComponent; }
    UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Animation")
    ULastFPSGrapplingAnimationComponent* GetGrapplingAnimationComponent() const
    {
        return GrapplingAnimationComponent;
    }
    UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Targeting")
    ULastFPSGrapplingTargetingComponent* GetGrapplingTargetingComponent() const
    {
        return GrapplingTargetingComponent;
    }
    virtual bool GetIsADS() const override { return bIsADS; }
    /** 스프린트 중인지. 애님 BP 에서 스레드 세이프하게 읽을 수 있다. */
    UFUNCTION(BlueprintPure, Category="LastFPS|Movement", meta=(BlueprintThreadSafe))
    FORCEINLINE bool GetIsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Movement", meta=(BlueprintThreadSafe))
    FORCEINLINE bool GetWantsToSprint() const { return bWantsToSprint; }
    FORCEINLINE bool GetWantsToWalk() const { return bWantsToWalk || CombatState == EMMCombatState::Attacking; }
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
    bool BeginTemporaryCameraEffect(
        UObject* RequestOwner,
        const FLastFPSTemporaryCameraEffectOptions& Options);
    void EndTemporaryCameraEffect(UObject* RequestOwner);

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
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data) override;
	virtual float ResolveMaxWalkSpeed(float AttributeMoveSpeed) const override;

    /** 초기 스폰·지연 스폰·리스폰 모두에서 로컬 플레이어의 기본 입력 매핑을 보장한다. */
    void EnsureDefaultInputMapping();

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

    const FLastFPSWeaponScopeSettings* GetActiveScopeSettings() const;
    void ShowScopeOverlay(bool bShow);
    void EnsureScopeOverlay();
    void DestroyScopeOverlay();

    void TickCameraInterp(float DeltaTime);
    void TickTemporaryCameraEffect(float DeltaTime);
    void RestoreTemporaryCameraEffect();
    void ApplyRotationModeSettings();
    bool ShouldUseControllerYawRotationMode() const;
    bool ShouldOrientRotationToMovement() const;
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

	/** 이 태그를 가진 효과가 하나 이상 활성화되면 카메라를 뒤로 이동합니다. */
	UPROPERTY(EditDefaultsOnly, Category="Camera|Speed Boost")
	FGameplayTag SpeedBoostCameraTag;

	UPROPERTY(EditDefaultsOnly, Category="Camera|Speed Boost", meta=(ClampMin="0.0", Units="cm"))
	float SpeedBoostArmLengthOffset = 120.f;

	/** 이 속도 이하에서는 카메라 거리 보너스를 적용하지 않습니다. */
	UPROPERTY(EditDefaultsOnly, Category="Camera|Speed Boost", meta=(ClampMin="0.0", Units="cm/s"))
	float SpeedBoostCameraStartSpeed = 600.f;

	/** 이 속도 이상에서 SpeedBoostArmLengthOffset 전체를 적용합니다. */
	UPROPERTY(EditDefaultsOnly, Category="Camera|Speed Boost", meta=(ClampMin="0.0", Units="cm/s"))
	float SpeedBoostCameraFullSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="Camera|Speed Boost", meta=(ClampMin="0.0"))
	float SpeedBoostCameraInterpSpeed = 3.f;

    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint")
    float SprintForwardInputThreshold = 0.5f;

    /** 스프린트 중 몸통 회전 속도 (Yaw, deg/s). 이동 방향으로 도는 속도. */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint", meta=(ClampMin="0.0"))
    float SprintRotationYaw = 600.f;

    /** 평상시 회전 속도 (Yaw, deg/s) */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint", meta=(ClampMin="0.0"))
    float NormalRotationYaw = 500.f;

    /** 속도 기반 자동 스프린트를 사용할지. 끄면 기존 입력(어빌리티) 방식만 동작. */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto")
    bool bAutoSprintBySpeed = true;

    /** 이 속도 이상이면 스프린트 진입 */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto", meta=(ClampMin="0.0", Units="cm/s"))
    float AutoSprintEnterSpeed = 1000.f;

    /** 이 속도 아래로 떨어지면 해제. 진입값보다 낮게 두어 경계 진동을 막는다. */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto", meta=(ClampMin="0.0", Units="cm/s"))
    float AutoSprintExitSpeed = 900.f;



    /** 조건을 벗어나도 이 시간만큼은 스프린트를 유지한다. 경사·벽 스침 대응. */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto", meta=(ClampMin="0.0", Units="s"))
    float AutoSprintExitGrace = 0.3f;

    /**
     * 사격 · 스킬 등으로 스프린트가 끊긴 뒤 이 시간 동안은 자동 재진입을 막는다.
     * 없으면 공격 상태가 풀리는 순간 바로 다시 켜져서 회전 모드가 깜빡인다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto", meta=(ClampMin="0.0", Units="s"))
    float AutoSprintReactivationDelay = 0.6f;

    /**
     * 정면 기준 이 각도 안쪽으로 입력했을 때만 스프린트한다.
     * 0 = 정면, 45 = 앞대각선, 90 = 좌우, 180 = 후진.
     * 90 이면 앞·앞대각선·좌우까지 허용하고 후진만 제외된다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Movement|Sprint|Auto", meta=(ClampMin="0.0", ClampMax="180.0"))
    float AutoSprintForwardAngle = 90.f;

    /** 입력이 정면 허용 각도 안에 있는지 (카메라 기준) */
    UFUNCTION(BlueprintPure, Category="LastFPS|Movement")
    bool IsForwardSprintInputAngle() const;

    /** 자동 스프린트 상태를 매 틱 평가한다. */
    void TickAutoSprint(float DeltaTime);

private:
    /** 해제 조건이 지속된 시간 */
    float AutoSprintExitTimer = 0.f;

    /** 자동 재진입 차단 잔여 시간 */
    float AutoSprintLockoutTimer = 0.f;

protected:

    UPROPERTY(EditDefaultsOnly, Category="Movement|Walk")
    float WalkMaxWalkSpeed = 250.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Weapon", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UWeaponComponent> WeaponComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Animation", meta=(AllowPrivateAccess="true"))
    TObjectPtr<ULastFPSGrapplingAnimationComponent> GrapplingAnimationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Grappling", meta=(AllowPrivateAccess="true"))
    TObjectPtr<ULastFPSGrapplingTargetingComponent> GrapplingTargetingComponent;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<ULastFPSInputConfig> InputConfig;

private:
    TWeakObjectPtr<UObject> ActiveCameraEffectOwner;
    float SavedCameraLagSpeed = 15.f;
    float SavedMotionBlurAmount = 0.f;
    bool bSavedCameraLagEnabled = false;
    bool bSavedMotionBlurOverride = false;
    float CameraEffectBlendOutDuration = 0.f;
    float CameraEffectBlendOutElapsed = 0.f;
    float CameraEffectBlendOutStartLagSpeed = 15.f;
    float CameraEffectBlendOutStartMotionBlurAmount = 0.f;
    bool bTemporaryCameraEffectOverrideActive = false;
    bool bTemporaryCameraEffectBlendOutActive = false;

    bool bIsADS = false;

    float ActiveAimSensitivityScale = 1.f;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ScopeOverlayWidget;

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
	bool bSpeedBoostCameraActive = false;
	bool bSpeedBoostCameraTransition = false;
	float CurrentSpeedBoostArmLengthOffset = 0.f;
	FDelegateHandle SpeedBoostCameraTagDelegateHandle;
	TWeakObjectPtr<UAbilitySystemComponent> SpeedBoostCameraASC;

    UFUNCTION()
    void OnRep_CombatState();

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);
	void BindSpeedBoostCameraTag();
	void UnbindSpeedBoostCameraTag();
	void HandleSpeedBoostCameraTagChanged(FGameplayTag Tag, int32 NewCount);
	void UpdateSpeedBoostCameraOffset(float EffectiveMoveSpeed);
	void RefreshCameraTargets();

    void HandleAbilityInput(const FInputActionValue& value, FGameplayTag InputID);
    bool ShouldCancelFireBeforeAbilityInput(FGameplayTag InputID) const;
    bool ShouldSkipAbilityCancelOnRelease(FGameplayTag InputID) const;
	void RefreshMaxWalkSpeed();

    void TickLocalMatchIntro();

    bool bLocalMatchIntroInputDisabled = false;
    bool bLocalMatchIntroFxStarted = false;
};
