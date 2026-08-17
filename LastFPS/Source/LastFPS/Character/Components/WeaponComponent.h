#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Utility/LastFPSEnumTypes.h"
#include "WeaponComponent.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
struct FStreamableHandle;
class AActor;
class ACharacter;
class AWeaponPickupActor;
class UAnimInstance;
class UGameplayEffect;
class USkeletalMesh;
class USkeletalMeshComponent;
class UParticleSystem;
class USoundBase;
class ULastFPSWeaponDefinition;
struct FLastFPSWeaponScopeSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedChanged, bool, bEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentAmmo, int32, MagazineCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReserveAmmoChanged, int32, ReserveAmmo);
// 리로드 표시용 신호. Duration은 리로드 총 소요 시간(초)이다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStarted, float, ReloadDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadFinished, bool, bCompleted);
// 활성 무기 슬롯이 바뀐 순간. HUD가 어떤 칸을 강조할지 판단한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSlotChanged, int32, ActiveSlotIndex);
// 슬롯 구성(어떤 무기를 어느 칸에 들었는지) 자체가 바뀐 순간.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponLoadoutChanged);

/**
 * 무기 슬롯 1칸의 상태.
 *
 * 비활성 슬롯의 탄약을 여기에 보관했다가 다시 꺼낼 때 복원한다. 활성 슬롯의 탄약은
 * CurrentMagazineAmmo/CurrentReserveAmmo 가 단일 출처이며, 이 구조체의 값은 전환 순간에만 갱신된다.
 */
USTRUCT()
struct FLastFPSWeaponSlotState
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<ULastFPSWeaponDefinition> Definition = nullptr;

    UPROPERTY()
    int32 StashedMagazineAmmo = 0;

    UPROPERTY()
    int32 StashedReserveAmmo = 0;

    /** 한 번이라도 꺼낸 적이 있는지. 첫 장착에서만 밸런스 기본 탄약으로 채우기 위한 구분이다. */
    UPROPERTY()
    bool bAmmoInitialized = false;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FTransform GetMuzzleTransform() const;
    bool CanFire() const;
    bool CanReload() const;
    bool TryConsumePredictedRound();
    void CompleteReload();

    // 리로드 UI 알림. 소요 시간은 컴포넌트가 소유한 ReloadDuration을 그대로 알리고,
    // 실제 리로드 진행·완료 타이밍은 GA_Reload가 타이머로 관리한다.
    void NotifyReloadStarted();
    void NotifyReloadFinished(bool bCompleted);
    float GetWeaponBaseDamage() const;
    void PlayFireEffects() const;
	void PlayFireCameraShake() const;
    void ApplyFireAimRecoil(bool bIsAiming);
    void SetWeaponHiddenForAbility(bool bHidden);

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void DetachMagazineToHand();

    UFUNCTION(BlueprintCallable, Category="Weapon|Reload")
    void RestoreMagazineToWeapon();
    void FireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, bool bAimingDownSights, TSubclassOf<UGameplayEffect> DamageEffectClass);

    /** 지금 발사하면 적용될 퍼짐 반각(도). 상태를 바꾸지 않으므로 크로스헤어 등 표시용으로 안전하다. */
    float GetCurrentSpreadHalfAngleDegrees(bool bAimingDownSights) const;

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransform(USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    UFUNCTION(BlueprintCallable, Category="Weapon|IK")
    bool GetLeftHandIKTransformForTarget(FName TargetName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const;

    /** 일반 무기 왼손 Two Bone IK에서 사용할 메시 컴포넌트 공간 팔꿈치 목표를 반환한다. */
    bool GetLeftHandIKJointTargetLocation(USkeletalMeshComponent* CharacterMesh, FVector& OutLocation) const;

    // 런타임 무기 장착 (서버에서 호출 → Multicast로 전체 적용)
    void EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass = nullptr);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void EquipWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void UnequipWeapon();

    /**
     * 슬롯 로드아웃 전체를 설정한다(서버 권한). 배틀 맵 스폰 시 EquipmentSubsystem 의 구성을 반영하는 진입점이다.
     * 1번 슬롯을 기본 활성 슬롯으로 사용하며, 목록 또는 1번 슬롯이 비어 있으면 맨손으로 시작한다.
     * 슬롯 수는 넘어온 배열 길이를 그대로 따른다.
     */
    void SetWeaponLoadout(const TArray<ULastFPSWeaponDefinition*>& SlotDefinitions);

    /**
     * 활성 슬롯 전환 요청. 소유 클라이언트에서 호출하면 서버 RPC 로 넘어가고,
     * 서버 권한에서 호출하면 즉시 적용된다. 유효하지 않은 인덱스나 빈 슬롯은 무시한다.
     */
    UFUNCTION(BlueprintCallable, Category="Weapon|Slot")
    void RequestWeaponSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category="Weapon|Slot")
    int32 GetActiveWeaponSlot() const { return ActiveWeaponSlot; }

    UFUNCTION(BlueprintPure, Category="Weapon|Slot")
    int32 GetWeaponSlotCount() const { return WeaponSlots.Num(); }

    UFUNCTION(BlueprintPure, Category="Weapon|Slot")
    ULastFPSWeaponDefinition* GetWeaponDefinitionForSlot(int32 SlotIndex) const;

    // 무기 장착/해제 시 HUD에 알림
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnWeaponEquippedChanged OnWeaponEquippedChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Slot")
    FOnWeaponSlotChanged OnWeaponSlotChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Slot")
    FOnWeaponLoadoutChanged OnWeaponLoadoutChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
    FOnWeaponAmmoChanged OnWeaponAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Ammo")
    FOnWeaponReserveAmmoChanged OnWeaponReserveAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Reload")
    FOnWeaponReloadStarted OnWeaponReloadStarted;

    UPROPERTY(BlueprintAssignable, Category="Weapon|Reload")
    FOnWeaponReloadFinished OnWeaponReloadFinished;

    UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
    int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

    UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
    int32 GetMagazineCapacity() const { return MagazineCapacity; }

    UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
    int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }

    /** 예비 탄약을 가득 채우기까지 남은 양. 0 이면 보급해도 들어가지 않는다. */
    int32 GetMissingReserveAmmo() const { return FMath::Max(0, StartingReserveAmmo - CurrentReserveAmmo); }

    UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
    bool HasReserveAmmo() const { return CurrentReserveAmmo > 0; }

    /** 서버 권한: 예비 탄약을 StartingReserveAmmo 까지 보급하고 채워진 양을 반환한다. */
    int32 Auth_AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintPure, Category="Weapon|Ammo")
    float GetReloadDuration() const { return ReloadDuration; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool HasWeapon() const { return CurrentWeapon != nullptr; }

    UFUNCTION(BlueprintCallable, Category="Weapon")
    EMMWeaponType GetWeaponType() const { return HasWeapon() ? WeaponType : EMMWeaponType::Unarmed; }

    UFUNCTION(BlueprintPure, Category="Weapon")
    ULastFPSWeaponDefinition* GetWeaponDefinition() const { return WeaponDefinition; }

    const FLastFPSWeaponScopeSettings* GetScopeSettings() const;

    // 무기 BP마다 Unarmed / Rifle / Pistol 지정 (Chooser Table 분기 입력)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponType, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponDefinition, Category="Weapon")
    TObjectPtr<ULastFPSWeaponDefinition> WeaponDefinition;

    // ── 에디터 설정 ──────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TObjectPtr<USkeletalMesh> WeaponSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSWeaponActor> WeaponActorClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    TSubclassOf<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName MuzzleSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName AttachSocketName = TEXT("WeaponSocket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName LeftHandIKSocketName = TEXT("LeftHandIK");

    /** 비전투 준비 자세에서 기본 왼손 IK 소켓 대신 사용할 무기 소켓이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName ReadyLeftHandIKSocketName = TEXT("LeftHandIK_Ready");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName LeftHandIKJointRootBoneName = TEXT("upperarm_l");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK", meta=(Units="cm"))
    FVector LeftHandIKJointTargetOffset = FVector(-15.f, -40.f, 5.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|IK")
    FName ReloadLeftHandIKTargetName = TEXT("Clip_Bone");

    /** WeaponBalance의 FireInterval에서 계산한 런타임 캐시이며 직접 설정하지 않는다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Weapon|Runtime")
    float FireRate = 0.1f;

    /** WeaponBalance의 AimTraceRange에서 계산한 런타임 캐시이며 직접 설정하지 않는다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Weapon|Runtime", meta=(Units="cm"))
    float AimTraceRange = 10000.f;

    /** 클라이언트 카메라 위치와 서버 시점 위치 사이에 허용할 최대 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="cm"))
    float MaxClientCameraLocationError = 200.f;

    /** 클라이언트 총구 위치와 서버 무기 총구 사이에 허용할 최대 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="cm"))
    float MaxClientMuzzleLocationError = 150.f;

    /** 네트워크 지터로 정상 발사가 거절되지 않도록 서버 발사 간격에 적용하는 허용 오차이다. */
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Network", meta=(ClampMin="0.0", Units="s"))
    float ServerFireIntervalTolerance = 0.02f;

    /** WeaponBalance의 Damage와 DamageElement에서 계산한 런타임 캐시이며 직접 설정하지 않는다. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Weapon|Runtime")
    FLastFPSDamageRange DamageRange;

    // ── 애니메이션 레이어 ──────────────────────────────────────────
    // 에디터에서 무기별 Layer ABP 클래스 할당 (ABP_Rifle_Layers 등)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TSubclassOf<UAnimInstance> UnarmedAnimLayerClass;

    // 발사 사운드/머즐 플래시는 WeaponComponent가 아니라 WeaponDefinition에서만 관리한다.

    UFUNCTION(BlueprintCallable, Category="Weapon|Debug")
    void TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_TestEquipWeapon();

    UFUNCTION(Server, Reliable)
    void Server_UnequipWeapon();

    UFUNCTION(Server, Reliable)
    // 디버그 드로잉 여부는 서버가 자기 설정(bDrawServerShotDebug)으로 판단한다.
    // 클라이언트가 서버의 디버그 렌더링을 켜게 두지 않는다.
    void Server_FireFromClientAim(FVector_NetQuantize ClientMuzzleLocation, FVector_NetQuantize ClientCameraLocation, FVector_NetQuantizeNormal ClientAimDirection, bool bAimingDownSights, TSubclassOf<UGameplayEffect> DamageEffectClass);

    UFUNCTION(Client, Reliable)
    void ClientCorrectMagazineAmmo(int32 ServerMagazineAmmo);

    // 발사 연출은 어빌리티가 소유 클라이언트에서만 재생하므로(GA_BasicShoot::Fire),
    // 다른 클라이언트에게는 서버가 전파해야 총구 화염과 발사음이 보인다.
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayFireEffects();

    UFUNCTION(Server, Reliable)
    void Server_RequestWeaponSlot(int32 SlotIndex);

    // 에디터에서 테스트할 픽업 BP 클래스 지정
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    TSubclassOf<AWeaponPickupActor> TestPickupClass;

    // 서버 히트스캔 궤적을 서버 화면에 그린다. 리슨 서버/PIE 진단용.
    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug")
    bool bDrawServerShotDebug = false;

    UPROPERTY(EditDefaultsOnly, Category="Weapon|Debug", meta=(ClampMin="0.0", ClampMax="5.0", Units="s"))
    float ServerShotDebugDuration = 1.f;
    
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon)
    TObjectPtr<ALastFPSWeaponActor> CurrentWeapon;

    UFUNCTION()
    void OnRep_CurrentWeapon();

    UFUNCTION()
    void OnRep_WeaponType();

    UFUNCTION()
    void OnRep_WeaponDefinition();

    /** OnRep_WeaponDefinition 의 실제 본문. 소프트 참조가 상주한 뒤에 실행한다. */
    void ApplyReplicatedWeaponDefinition();

    /** 복제로 받은 정의의 장착 참조를 비동기로 상주시키는 핸들이다. */
    TSharedPtr<FStreamableHandle> WeaponDefinitionPreloadHandle;

    /**
     * 슬롯 구성. 소유 클라이언트에만 복제한다 — HUD 표기와 예비 탄약은 본인에게만 필요한 정보이고,
     * 다른 클라이언트는 실제로 든 무기(CurrentWeapon)만 보면 충분하기 때문이다.
     */
    UPROPERTY(ReplicatedUsing=OnRep_WeaponSlots)
    TArray<FLastFPSWeaponSlotState> WeaponSlots;

    UFUNCTION()
    void OnRep_WeaponSlots();

    UPROPERTY(ReplicatedUsing=OnRep_ActiveWeaponSlot)
    int32 ActiveWeaponSlot = 0;

    UFUNCTION()
    void OnRep_ActiveWeaponSlot();

    /** 서버 권한에서 슬롯을 실제로 전환한다. 현재 슬롯의 탄약을 보관한 뒤 새 슬롯을 장착한다. */
    void ApplyWeaponSlot(int32 SlotIndex);

    /** 슬롯 배열은 유지한 채 현재 손에 든 무기 상태만 맨손으로 되돌린다. */
    void ClearCurrentWeaponState();

    /** 활성 슬롯의 현재 탄약을 슬롯 상태로 옮긴다. 전환·해제 직전에만 호출한다. */
    void StashActiveSlotAmmo();

    UPROPERTY(ReplicatedUsing=OnRep_CurrentMagazineAmmo)
    int32 CurrentMagazineAmmo = 0;

    UFUNCTION()
    void OnRep_CurrentMagazineAmmo();

    UPROPERTY(ReplicatedUsing=OnRep_CurrentReserveAmmo)
    int32 CurrentReserveAmmo = 0;

    UFUNCTION()
    void OnRep_CurrentReserveAmmo();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_DetachMagazineToHand();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_RestoreMagazineToWeapon();

    void ApplyEquip(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass);
    void ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition);
    bool ApplyWeaponDefinitionValues(const ULastFPSWeaponDefinition* NewDefinition);
    void ApplyAnimLayerClass(TSubclassOf<UAnimInstance> AnimLayerClass) const;
    TSubclassOf<UAnimInstance> ResolveCurrentAnimLayerClass() const;

    /** 소유 캐릭터의 AnimInstance 가 재생성되면 현재 무기 기준으로 애님 레이어를 다시 링크한다. */
    void HandleOwnerAnimInstanceRecreated();

    void AttachWeaponToOwner(ALastFPSWeaponActor* WeaponActor);
    void ApplyWeaponVisibilityOverride();
    ALastFPSWeaponActor* SpawnWeaponActor(USkeletalMesh* NewMesh, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass, ULastFPSWeaponDefinition* Definition = nullptr);
    void DestroyCurrentWeapon();
    void HandleFireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, bool bAimingDownSights, TSubclassOf<UGameplayEffect> DamageEffectClass);
    // 서버 권한에서 산탄 한 발(펠릿 하나)의 히트 판정·투사체 스폰·데미지 적용을 수행한다. 방향만 다를 뿐 나머지 맥락은 발사 1회 내에서 공유한다.
    void FireSinglePelletFromServer(ACharacter& Character, const FVector& TraceStart, const FVector& MuzzleLocation, const FVector& PelletDirection, TSubclassOf<UGameplayEffect> DamageEffectClass);
    // 경과 시간만큼 감쇠시킨 현재 누적 확산(도). 상태는 건드리지 않는다.
    float PeekSpreadBloomDegrees() const;
    // 발사 1회분 누적 확산을 반영하고 그 발사에 적용할 총 퍼짐 반각(도)을 돌려준다.
    float ConsumeSpreadHalfAngleForShot(bool bAimingDownSights);
    bool ValidateClientMuzzleLocation(const FVector& ClientMuzzleLocation) const;
    FVector ResolveValidatedTraceStart(const ACharacter& Character, const FVector& ClientCameraLocation) const;
    bool TryConsumeServerFirePermission();
    void SetCurrentMagazineAmmo(int32 NewMagazineAmmo);
    void SetCurrentReserveAmmo(int32 NewReserveAmmo);
    void NotifyAmmoChanged();
    void ResetPendingAimRecoil();
    void ResetAimRecoilSequence();
    void ApplyDetachMagazineVisual();
    void ApplyRestoreMagazineVisual();

    UPROPERTY(Transient)
    TObjectPtr<AActor> DetachedMagazineVisual;

    UPROPERTY(Transient)
    TObjectPtr<ALastFPSWeaponActor> MagazineSourceWeapon;

    FName HiddenMagazineBoneName;

    int32 WeaponHiddenOverrideCount = 0;
    float PendingAimRecoilPitch = 0.f;
    float PendingAimRecoilYaw = 0.f;
    float RecoverableAimRecoilPitch = 0.f;
    float RecoverableAimRecoilYaw = 0.f;
    double LastAimRecoilTimeSeconds = 0.0;
    double NextAllowedServerFireTimeSeconds = 0.0;
    int32 MagazineCapacity = 30;
    int32 StartingReserveAmmo = 90;
    float ReloadDuration = 2.f;

    // 한 번의 발사(방아쇠 1회)에서 생성할 산탄 개수. 1이면 단일탄, 2 이상이면 샷건처럼 동시에 여러 발이 나간다.
    int32 PelletsPerShot = 1;
    // 산탄 퍼짐 원뿔의 반각(도). 0이면 퍼짐 없이 조준 방향으로 곧게 나간다.
    float SpreadHalfAngleDegrees = 0.f;
    // ADS 조준 중 퍼짐 반각에 곱할 배율. 1이면 허리사격과 동일하다.
    float SpreadADSMultiplier = 1.f;
    // 연사 누적 확산 설정(도, 도, 도/초). PerShot이 0이면 누적 확산이 비활성이다.
    float SpreadBloomPerShotDegrees = 0.f;
    float SpreadBloomMaxDegrees = 0.f;
    float SpreadBloomRecoveryPerSecond = 0.f;
    // 누적 확산 런타임 상태. Tick 없이 마지막 갱신 시각과의 경과로 감쇠를 계산한다.
    float SpreadBloomDegrees = 0.f;
    float SpreadBloomUpdatedTimeSeconds = 0.f;
    bool bHasFiredAimRecoil = false;

    /** 소유 캐릭터의 AnimInstance 재생성 알림 구독. EndPlay 에서 반드시 해제한다. */
    FDelegateHandle AnimInstanceRecreatedHandle;
};
