#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSDamageDirectionIndicatorWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UWidget;

/** 하나의 공격 방향 표시와 표시 수명을 관리한다. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class LASTFPS_API ULastFPSDamageDirectionIndicatorWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 월드 방향을 저장하고 표시를 시작한다. 유효하지 않은 방향이면 false를 반환한다. */
    bool InitializeDamageDirection(const FVector& DamageSourceDirection);

    /** HUD 갱신 주기에서 호출한다. 표시 수명이 끝났으면 false를 반환한다. */
    bool AdvanceIndicator(float DeltaTime, const FRotator& ViewRotation);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** 아래쪽 피벗을 가진 회전 전용 컨테이너다. 이미지 자체의 피벗과 회전을 분리한다. */
    UPROPERTY(BlueprintReadOnly, Category="HUD|Damage Direction", meta=(BindWidget))
    TObjectPtr<UWidget> DamageDirectionIndicatorRotationRoot;

    UPROPERTY(BlueprintReadOnly, Category="HUD|Damage Direction", meta=(BindWidget))
    TObjectPtr<UImage> DamageDirectionIndicatorImage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Damage Direction", meta=(ClampMin="0.05", Units="s"))
    float DamageDirectionShrinkDuration = 0.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Damage Direction", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DamageDirectionMinimumProgress = 0.05f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Damage Direction", meta=(ClampMin="0.0", Units="s"))
    float DamageDirectionHoldDuration = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category="HUD|Damage Direction|Material")
    FName DamageDirectionProgressParameterName = TEXT("Progress");

private:
    void InitializeDamageDirectionMaterial();
    void SetDamageDirectionProgress(float Progress);
    void UpdateDamageDirectionAngle(const FRotator& ViewRotation);

    TWeakObjectPtr<UMaterialInstanceDynamic> DamageDirectionMaterial;
    FVector DamageSourceDirection = FVector::ZeroVector;
    float ElapsedTime = 0.f;
    bool bMaterialWarningLogged = false;
};
