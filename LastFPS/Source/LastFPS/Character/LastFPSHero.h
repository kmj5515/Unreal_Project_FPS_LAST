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

    // GAS 어빌리티에서 카메라 줌을 제어할 수 있도록 공개
    void SetADS(bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void GiveDefaultAbilities() override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    // 네이티브 전용 입력 처리
    void StartScoreboard();
    void StopScoreboard();

    // 공통 GAS 입력 처리
    void TryActivateAbilityByTag(FGameplayTag AbilityTag);
    void CancelAbilityByTag(FGameplayTag AbilityTag);
    void InputPressed(FGameplayTag InputID);
    void InputReleased(FGameplayTag InputID);

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

    void HandleAbilityInput(const FInputActionValue& value, FGameplayTag InputID);

    void TickLocalMatchIntro();

    bool bLocalMatchIntroInputDisabled = false;
    bool bLocalMatchIntroFxStarted = false;
};
