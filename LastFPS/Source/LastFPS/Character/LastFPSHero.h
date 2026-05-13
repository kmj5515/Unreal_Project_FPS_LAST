#pragma once

#include "CoreMinimal.h"
#include "Character/LastFPSCharacterBase.h"
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

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayWeaponFireEffects();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
    virtual bool GetIsADS() const override { return bIsADS; }

protected:
    virtual void BeginPlay() override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartADS();
    void StopADS();

    void StartJump();
    void StopJump();

    void StartFire();
    void StopFire();

    void StartSkill1();
    void StartSkill2();
    void StartUltimate();

    void StartScoreboard();
    void StopScoreboard();
    void TryActivateAbilityByTag(const FGameplayTag& AbilityTag);
    void CancelAbilityByTag(const FGameplayTag& AbilityTag);

    void TickCameraInterp(float DeltaTime);

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
    float ADSArmLength = 120.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    FVector ADSSocketOffset = FVector(0.f, 55.f, 25.f);

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSFOV = 75.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera|ADS")
    float ADSInterpSpeed = 10.f;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float CameraLagSpeed = 15.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<UWeaponComponent> WeaponComponent;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<ULastFPSInputConfig> InputConfig;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|MatchIntro")
    TObjectPtr<UAnimMontage> MatchIntroMontage;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|MatchIntro")
    TSubclassOf<UCameraShakeBase> MatchIntroCameraShake;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|MatchIntro")
    float MatchIntroCameraShakeScale = 1.f;

private:
    bool bIsADS = false;

    float TargetArmLength;
    FVector TargetSocketOffset;
    float TargetFOV;

    void TryBindNativeTriggered(UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
        void(ALastFPSHero::*Func)(const FInputActionValue&));
    void TryBindNativeStartStop(UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
        void(ALastFPSHero::*StartFunc)(), void(ALastFPSHero::*StopFunc)());
    void TryBindAbilityStart(UEnhancedInputComponent* EIC, const FGameplayTag& Tag,
        void(ALastFPSHero::*Func)());

    void TickLocalMatchIntro();

    bool bLocalMatchIntroInputDisabled = false;
    bool bLocalMatchIntroFxStarted = false;
};
