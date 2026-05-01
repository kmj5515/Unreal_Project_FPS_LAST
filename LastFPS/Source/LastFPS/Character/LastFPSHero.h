#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
#include "LastFPSHero.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class ULastFPSInputConfig;
class UWeaponComponent;
struct FInputActionValue;

UCLASS()
class LASTFPS_API ALastFPSHero : public ALastFPSCharacterBase
{
    GENERATED_BODY()

public:
    ALastFPSHero();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
    virtual bool GetIsADS() const override { return bIsADS; }

protected:
    virtual void BeginPlay() override;

    // ── 이동 / 시점 ──────────────────────────────────────────
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartADS();
    void StopADS();

    // ── 점프 / 더블점프 ──────────────────────────────────────
    void StartJump();
    void StopJump();

    // ── 사격 ─────────────────────────────────────────────────
    void StartFire();
    void StopFire();

    // ── 카메라 보간 ───────────────────────────────────────────
    void TickCameraInterp(float DeltaTime);

    // ── 카메라 컴포넌트 ───────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    // 기본 시점 설정
    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    float DefaultArmLength = 300.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    FVector DefaultSocketOffset = FVector(0.f, 60.f, 20.f);

    UPROPERTY(EditDefaultsOnly, Category="Camera|Default")
    float DefaultFOV = 90.f;

    // ADS 시점 설정 — 카메라가 어깨 쪽으로 당겨져 허리 위만 보임
    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSArmLength = 120.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    FVector ADSSocketOffset = FVector(0.f, 55.f, 25.f);

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSFOV = 75.f;

    // 보간 속도 (높을수록 빠르게 전환)
    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSInterpSpeed = 10.f;

    // ── 무기 ─────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<UWeaponComponent> WeaponComponent;

    // ── 입력 설정 ─────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<ULastFPSInputConfig> InputConfig;

private:
    bool bIsADS = false;

    // 현재 보간 중인 목표값
    float TargetArmLength;
    FVector TargetSocketOffset;
    float TargetFOV;
};
