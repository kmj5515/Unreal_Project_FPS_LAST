#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSAmmoPresenter.generated.h"

class UTextBlock;
class UWidget;
class UWeaponComponent;

/**
 * HUD 내 탄약(현재 탄약, 예비 탄약) 표시를 관리하는 Presenter이다.
 * UWeaponComponent의 탄약 및 무기 장착 이벤트를 수신하여 UI 텍스트 위젯을 갱신한다.
 */
UCLASS()
class LASTFPS_API ULastFPSAmmoPresenter final : public UObject
{
    GENERATED_BODY()

public:
    /**
     * UI 위젯 바인딩 초기화. 각 텍스트 위젯은 nullptr일 수 있으며(BindWidgetOptional),
     * 유효한 위젯에만 탄약 수치를 반영한다.
     */
    void Initialize(
        UTextBlock* InCurrentAmmoText,
        UTextBlock* InReserveAmmoText,
        UTextBlock* InCombinedAmmoText = nullptr,
        UWidget* InAmmoInfoLayer = nullptr);

    /** 무기 컴포넌트 델리게이트 바인딩 및 초기 탄약 수치 반영 */
    void BindToWeaponComponent(UWeaponComponent* InWeaponComponent);

    /** 바인딩 해제 및 상태 리셋 */
    void Reset();

private:
    UFUNCTION()
    void HandleAmmoChanged(int32 CurrentAmmo, int32 MagazineCapacity);

    UFUNCTION()
    void HandleReserveAmmoChanged(int32 ReserveAmmo);

    UFUNCTION()
    void HandleWeaponEquippedChanged(bool bEquipped);

    void UpdateDisplay();

    UPROPERTY(Transient)
    TWeakObjectPtr<UTextBlock> CurrentAmmoText;

    UPROPERTY(Transient)
    TWeakObjectPtr<UTextBlock> ReserveAmmoText;

    UPROPERTY(Transient)
    TWeakObjectPtr<UTextBlock> CombinedAmmoText;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWidget> AmmoInfoLayer;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWeaponComponent> WeaponComponent;

    int32 CachedCurrentAmmo = 0;
    int32 CachedMagazineCapacity = 0;
    int32 CachedReserveAmmo = 0;
    bool bHasWeapon = false;
};
