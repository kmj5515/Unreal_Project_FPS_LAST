#include "Character/Components/WeaponComponent.h"

#include "Net/UnrealNetwork.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/Components/LastFPSWeakpointComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapons/LastFPSWeaponActor.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Data/Tables/LastFPSWeaponBalanceData.h"
#include "Weapons/WeaponPickupActor.h"
#include "Weapons/LastFPSWeaponDataSubsystem.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Utility/LastFPSTags.h"

namespace
{
    constexpr float AimRecoilCompletionToleranceDegrees = 0.01f;
    constexpr float MaximumServerDebugShotDurationSeconds = 5.f;
    constexpr float MinimumServerFireIntervalSeconds = 0.01f;
    constexpr int32 DefaultBattleWeaponSlotIndex = 0;
}

UWeaponComponent::UWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UWeaponComponent, CurrentWeapon);
    DOREPLIFETIME(UWeaponComponent, WeaponType);
    DOREPLIFETIME(UWeaponComponent, WeaponAnimLayerClass);
    DOREPLIFETIME(UWeaponComponent, WeaponDefinition);
    DOREPLIFETIME_CONDITION(UWeaponComponent, CurrentMagazineAmmo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UWeaponComponent, CurrentReserveAmmo, COND_OwnerOnly);
    // 슬롯 구성과 보관 탄약은 소유자 HUD 전용 정보라 대역폭을 아껴 소유 클라이언트에만 보낸다.
    DOREPLIFETIME_CONDITION(UWeaponComponent, WeaponSlots, COND_OwnerOnly);
    DOREPLIFETIME(UWeaponComponent, ActiveWeaponSlot);
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(false);

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (WeaponDefinition)
        {
            ApplyWeaponDefinition(WeaponDefinition);
        }
        else if (WeaponSkeletalMesh)
        {
            ApplyEquip(WeaponSkeletalMesh, WeaponType, WeaponAnimLayerClass, WeaponActorClass);
        }
        else
        {
            ApplyAnimLayerClass(UnarmedAnimLayerClass);
            OnWeaponEquippedChanged.Broadcast(false);
        }
    }
    // 클라이언트는 OnRep_CurrentWeapon에서 attach + 브로드캐스트
}

void UWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ApplyRestoreMagazineVisual();
    Super::EndPlay(EndPlayReason);
}

bool UWeaponComponent::CanFire() const
{
    return CurrentWeapon != nullptr && CurrentMagazineAmmo > 0;
}

bool UWeaponComponent::CanReload() const
{
    return CurrentWeapon != nullptr
        && MagazineCapacity > 0
        && CurrentMagazineAmmo < MagazineCapacity
        && CurrentReserveAmmo > 0;
}

bool UWeaponComponent::TryConsumePredictedRound()
{
    if (!CanFire())
    {
        return false;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        return false;
    }

    // 권한 인스턴스는 실제 발사 허용 검사에서만 탄약을 차감한다.
    if (OwnerCharacter->HasAuthority())
    {
        return true;
    }

    if (!OwnerCharacter->IsLocallyControlled())
    {
        return false;
    }

    SetCurrentMagazineAmmo(CurrentMagazineAmmo - 1);
    return true;
}

void UWeaponComponent::CompleteReload()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || (!OwnerCharacter->HasAuthority() && !OwnerCharacter->IsLocallyControlled()))
    {
        return;
    }

    if (CanReload())
    {
        const int32 MissingAmmo = MagazineCapacity - CurrentMagazineAmmo;
        const int32 AmmoToLoad = FMath::Min(MissingAmmo, CurrentReserveAmmo);
        SetCurrentMagazineAmmo(CurrentMagazineAmmo + AmmoToLoad);
        SetCurrentReserveAmmo(CurrentReserveAmmo - AmmoToLoad);
    }

    if (OwnerCharacter->HasAuthority())
    {
        NextAllowedServerFireTimeSeconds = 0.0;
        OwnerCharacter->ForceNetUpdate();
    }
}

void UWeaponComponent::NotifyReloadStarted()
{
    // 소요 시간은 이 컴포넌트가 데이터로부터 이미 보유한 값이다. 외부에서 받아오지 않고 그대로 알린다.
    // 실제 리로드 진행·완료는 GA_Reload가 타이머로 관리한다.
    OnWeaponReloadStarted.Broadcast(FMath::Max(ReloadDuration, 0.f));
}

void UWeaponComponent::NotifyReloadFinished(bool bCompleted)
{
    OnWeaponReloadFinished.Broadcast(bCompleted);
}

FTransform UWeaponComponent::GetMuzzleTransform() const
{
    if (!CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetMuzzleTransform failed: CurrentWeapon is null"));
        return GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity;
    }

    return CurrentWeapon->GetMuzzleTransform(MuzzleSocketName);
}

void UWeaponComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const bool bHasPendingKick = !FMath::IsNearlyZero(PendingAimRecoilPitch, KINDA_SMALL_NUMBER)
        || !FMath::IsNearlyZero(PendingAimRecoilYaw, KINDA_SMALL_NUMBER);
    const bool bHasRecoverableRecoil = !FMath::IsNearlyZero(RecoverableAimRecoilPitch, KINDA_SMALL_NUMBER)
        || !FMath::IsNearlyZero(RecoverableAimRecoilYaw, KINDA_SMALL_NUMBER);
    if (!bHasPendingKick && !bHasRecoverableRecoil)
    {
        ResetPendingAimRecoil();
        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    APlayerController* PlayerController = OwnerCharacter && OwnerCharacter->IsLocallyControlled()
        ? Cast<APlayerController>(OwnerCharacter->GetController())
        : nullptr;
    if (!PlayerController || !WeaponDefinition)
    {
        ResetPendingAimRecoil();
        return;
    }

    const FLastFPSWeaponAimRecoilSettings& Recoil = WeaponDefinition->AimRecoil;
    FRotator RecoilRotation = PlayerController->GetControlRotation();
    bool bRotationChanged = false;

    if (bHasPendingKick)
    {
        const float KickSpeed = FMath::Max(Recoil.InterpolationSpeed, 0.01f);
        const float KickAlpha = FMath::Clamp(DeltaTime * KickSpeed, 0.f, 1.f);
        float PitchKickDelta = PendingAimRecoilPitch * KickAlpha;
        float YawKickDelta = PendingAimRecoilYaw * KickAlpha;
        if (FMath::Abs(PendingAimRecoilPitch - PitchKickDelta) <= AimRecoilCompletionToleranceDegrees)
        {
            PitchKickDelta = PendingAimRecoilPitch;
        }
        if (FMath::Abs(PendingAimRecoilYaw - YawKickDelta) <= AimRecoilCompletionToleranceDegrees)
        {
            YawKickDelta = PendingAimRecoilYaw;
        }

        RecoilRotation.Pitch = FRotator::NormalizeAxis(RecoilRotation.Pitch + PitchKickDelta);
        RecoilRotation.Yaw = FRotator::NormalizeAxis(RecoilRotation.Yaw + YawKickDelta);
        PendingAimRecoilPitch -= PitchKickDelta;
        PendingAimRecoilYaw -= YawKickDelta;
        const float RecoveryRatio = FMath::Clamp(Recoil.RecoveryRatio, 0.f, 1.f);
        RecoverableAimRecoilPitch += PitchKickDelta * RecoveryRatio;
        RecoverableAimRecoilYaw += YawKickDelta * RecoveryRatio;
        bRotationChanged = true;
    }
    else
    {
        const float RecoverySpeed = FMath::Max(Recoil.RecoveryInterpolationSpeed, 0.01f);
        const float RecoveryAlpha = FMath::Clamp(DeltaTime * RecoverySpeed, 0.f, 1.f);
        const float PitchRecoveryDelta = RecoverableAimRecoilPitch * RecoveryAlpha;
        const float YawRecoveryDelta = RecoverableAimRecoilYaw * RecoveryAlpha;

        RecoilRotation.Pitch = FRotator::NormalizeAxis(RecoilRotation.Pitch - PitchRecoveryDelta);
        RecoilRotation.Yaw = FRotator::NormalizeAxis(RecoilRotation.Yaw - YawRecoveryDelta);
        RecoverableAimRecoilPitch -= PitchRecoveryDelta;
        RecoverableAimRecoilYaw -= YawRecoveryDelta;
        bRotationChanged = !FMath::IsNearlyZero(PitchRecoveryDelta)
            || !FMath::IsNearlyZero(YawRecoveryDelta);
    }

    if (bRotationChanged)
    {
        PlayerController->SetControlRotation(RecoilRotation);
    }
}

float UWeaponComponent::GetWeaponBaseDamage() const
{
    if (!HasWeapon())
    {
        return 0.f;
    }

    return FMath::Max((DamageRange.MinDamage + DamageRange.MaxDamage) * 0.5f, 0.f);
}

void UWeaponComponent::PlayFireEffects() const
{
    if (!CurrentWeapon || !CurrentWeapon->GetWeaponMesh())
    {
        return;
    }

    CurrentWeapon->PlayFireAnimation();

    // 사운드/머즐 플래시는 WeaponDefinition을 통해서만 재생 (WeaponActor가 Definition에서 직접 조회)
    CurrentWeapon->PlayFireEffects(MuzzleSocketName);
}

void UWeaponComponent::PlayFireCameraShake() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter
		|| !OwnerCharacter->IsLocallyControlled()
		|| !WeaponDefinition
		|| !WeaponDefinition->FireCameraShakeClass
		|| WeaponDefinition->FireCameraShakeScale <= 0.f)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	PlayerController->PlayerCameraManager->StartCameraShake(
		WeaponDefinition->FireCameraShakeClass,
		WeaponDefinition->FireCameraShakeScale,
		ECameraShakePlaySpace::CameraLocal);
}

void UWeaponComponent::ApplyFireAimRecoil(bool bIsAiming)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter
        || !OwnerCharacter->IsLocallyControlled()
        || !WeaponDefinition
        || !WeaponDefinition->AimRecoil.bEnabled)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PlayerController)
    {
        return;
    }

    const FLastFPSWeaponAimRecoilSettings& Recoil = WeaponDefinition->AimRecoil;
    const float Strength = FMath::Max(Recoil.Strength, 0.f);
    if (Strength <= 0.f)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
    const double TimeSinceLastShot = CurrentTimeSeconds - LastAimRecoilTimeSeconds;
    const bool bIsFirstShot = !bHasFiredAimRecoil
        || TimeSinceLastShot >= FMath::Max(static_cast<double>(Recoil.FirstShotResetInterval), 0.0);
    LastAimRecoilTimeSeconds = CurrentTimeSeconds;
    bHasFiredAimRecoil = true;

    const float Randomness = FMath::Clamp(Recoil.RandomnessRatio, 0.f, 1.f);
    const float AimMultiplier = bIsAiming ? FMath::Max(Recoil.ADSMultiplier, 0.f) : 1.f;
    const float FirstShotMultiplier = bIsFirstShot
        ? FMath::Max(Recoil.FirstShotStrengthMultiplier, 0.f)
        : 1.f;
    const float VerticalRecoil = Strength
        * FirstShotMultiplier
        * FMath::FRandRange(1.f - Randomness, 1.f + Randomness)
        * AimMultiplier;
    const float HorizontalRecoil = Strength
        * FirstShotMultiplier
        * FMath::Max(Recoil.HorizontalRatio, 0.f)
        * FMath::FRandRange(-1.f, 1.f)
        * AimMultiplier;

    PendingAimRecoilPitch += VerticalRecoil;
    PendingAimRecoilYaw += HorizontalRecoil;
    SetComponentTickEnabled(true);
}

void UWeaponComponent::ResetPendingAimRecoil()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    APlayerController* PlayerController = OwnerCharacter && OwnerCharacter->IsLocallyControlled()
        ? Cast<APlayerController>(OwnerCharacter->GetController())
        : nullptr;
    if (PlayerController
        && (!FMath::IsNearlyZero(RecoverableAimRecoilPitch)
            || !FMath::IsNearlyZero(RecoverableAimRecoilYaw)))
    {
        FRotator RestoredRotation = PlayerController->GetControlRotation();
        RestoredRotation.Pitch = FRotator::NormalizeAxis(
            RestoredRotation.Pitch - RecoverableAimRecoilPitch);
        RestoredRotation.Yaw = FRotator::NormalizeAxis(
            RestoredRotation.Yaw - RecoverableAimRecoilYaw);
        PlayerController->SetControlRotation(RestoredRotation);
    }

    PendingAimRecoilPitch = 0.f;
    PendingAimRecoilYaw = 0.f;
    RecoverableAimRecoilPitch = 0.f;
    RecoverableAimRecoilYaw = 0.f;
    SetComponentTickEnabled(false);
}

void UWeaponComponent::ResetAimRecoilSequence()
{
    LastAimRecoilTimeSeconds = 0.0;
    bHasFiredAimRecoil = false;
}

void UWeaponComponent::SetWeaponHiddenForAbility(bool bHidden)
{
    WeaponHiddenOverrideCount = bHidden
        ? WeaponHiddenOverrideCount + 1
        : FMath::Max(WeaponHiddenOverrideCount - 1, 0);

    ApplyWeaponVisibilityOverride();
}

void UWeaponComponent::DetachMagazineToHand()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (OwnerActor->HasAuthority())
    {
        Multicast_DetachMagazineToHand();
        return;
    }

    // 예측 클라이언트는 서버 왕복을 기다리지 않고 동일한 시각 상태를 먼저 적용합니다.
    ApplyDetachMagazineVisual();
}

void UWeaponComponent::RestoreMagazineToWeapon()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        ApplyRestoreMagazineVisual();
        return;
    }

    if (OwnerActor->HasAuthority())
    {
        Multicast_RestoreMagazineToWeapon();
        return;
    }

    // 서버 Multicast가 도착해도 복구 함수가 멱등적으로 동작하므로 중복 호출은 안전합니다.
    ApplyRestoreMagazineVisual();
}

void UWeaponComponent::Multicast_DetachMagazineToHand_Implementation()
{
    ApplyDetachMagazineVisual();
}

void UWeaponComponent::Multicast_RestoreMagazineToWeapon_Implementation()
{
    ApplyRestoreMagazineVisual();
}

void UWeaponComponent::ApplyDetachMagazineVisual()
{
    if (DetachedMagazineVisual
        || !CurrentWeapon
        || !WeaponDefinition
        || (GetOwner() && GetOwner()->GetNetMode() == NM_DedicatedServer))
    {
        return;
    }

    const FLastFPSWeaponMagazineVisualSettings& Settings = WeaponDefinition->MagazineVisual;
    USkeletalMeshComponent* WeaponMesh = CurrentWeapon->GetWeaponMesh();
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    USkeletalMeshComponent* CharacterMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
    UWorld* World = GetWorld();
    if (!World || !WeaponMesh || !CharacterMesh || !Settings.DetachedMagazineActorClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("탄창 분리 실패: Owner=%s Weapon=%s Definition=%s에 필요한 시각 설정이 없습니다."),
            GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
            CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("None"),
            WeaponDefinition ? *WeaponDefinition->GetName() : TEXT("None"));
        return;
    }

    if (Settings.WeaponMagazineBoneName.IsNone()
        || WeaponMesh->GetBoneIndex(Settings.WeaponMagazineBoneName) == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("탄창 분리 실패: 무기 '%s'에서 탄창 본 '%s'을 찾지 못했습니다."),
            *CurrentWeapon->GetName(),
            *Settings.WeaponMagazineBoneName.ToString());
        return;
    }

    if (Settings.HandSocketName.IsNone() || !CharacterMesh->DoesSocketExist(Settings.HandSocketName))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("탄창 분리 실패: 캐릭터 메시 '%s'에서 손 소켓 '%s'을 찾지 못했습니다."),
            *CharacterMesh->GetName(),
            *Settings.HandSocketName.ToString());
        return;
    }

    const FTransform SocketTransform = CharacterMesh->GetSocketTransform(Settings.HandSocketName, RTS_World);
    AActor* MagazineVisual = nullptr;
    if (ULastFPSActorPoolSubsystem* Pool =
        World->GetSubsystem<ULastFPSActorPoolSubsystem>())
    {
        MagazineVisual = Pool->AcquireActorByClass(
            Settings.DetachedMagazineActorClass,
            SocketTransform,
            OwnerCharacter,
            OwnerCharacter);
    }
    if (!MagazineVisual)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = OwnerCharacter;
        SpawnParameters.Instigator = OwnerCharacter;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MagazineVisual = World->SpawnActor<AActor>(
            Settings.DetachedMagazineActorClass,
            SocketTransform,
            SpawnParameters);
    }
    if (!MagazineVisual)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("탄창 분리 실패: Definition '%s'의 탄창 BP를 생성하지 못했습니다."),
            *WeaponDefinition->GetName());
        return;
    }

    MagazineVisual->SetReplicates(false);
    if (!MagazineVisual->AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        Settings.HandSocketName))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("탄창 분리 실패: 생성한 탄창 '%s'을 손 소켓 '%s'에 부착하지 못했습니다."),
            *MagazineVisual->GetName(),
            *Settings.HandSocketName.ToString());
        if (ULastFPSActorPoolSubsystem* Pool =
            World->GetSubsystem<ULastFPSActorPoolSubsystem>();
            !Pool || !Pool->ReleaseActor(MagazineVisual))
        {
            MagazineVisual->Destroy();
        }
        return;
    }

    MagazineVisual->SetActorRelativeTransform(Settings.HandAttachmentOffset);
    WeaponMesh->HideBoneByName(Settings.WeaponMagazineBoneName, EPhysBodyOp::PBO_None);

    DetachedMagazineVisual = MagazineVisual;
    MagazineSourceWeapon = CurrentWeapon;
    HiddenMagazineBoneName = Settings.WeaponMagazineBoneName;
}

void UWeaponComponent::ApplyRestoreMagazineVisual()
{
    if (IsValid(DetachedMagazineVisual))
    {
        if (ULastFPSActorPoolSubsystem* Pool =
            GetWorld() ? GetWorld()->GetSubsystem<ULastFPSActorPoolSubsystem>() : nullptr;
            !Pool || !Pool->ReleaseActor(DetachedMagazineVisual))
        {
            DetachedMagazineVisual->Destroy();
        }
    }
    DetachedMagazineVisual = nullptr;

    if (IsValid(MagazineSourceWeapon) && !HiddenMagazineBoneName.IsNone())
    {
        if (USkeletalMeshComponent* WeaponMesh = MagazineSourceWeapon->GetWeaponMesh())
        {
            WeaponMesh->UnHideBoneByName(HiddenMagazineBoneName);
        }
    }

    MagazineSourceWeapon = nullptr;
    HiddenMagazineBoneName = NAME_None;
}

void UWeaponComponent::FireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration)
{
    // 예측 차감으로 마지막 탄이 0이 된 직후에도 서버 발사 요청은 전달되어야 한다.
    if (!HasWeapon())
    {
        return;
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        HandleFireFromClientAim(ClientMuzzleLocation, ClientCameraLocation, ClientAimDirection, DamageEffectClass, bDrawDebugShot, DebugShotDuration);
        return;
    }

    Server_FireFromClientAim(ClientMuzzleLocation, ClientCameraLocation, ClientAimDirection.GetSafeNormal(), DamageEffectClass, bDrawDebugShot, DebugShotDuration);
}

void UWeaponComponent::Server_FireFromClientAim_Implementation(FVector_NetQuantize ClientMuzzleLocation, FVector_NetQuantize ClientCameraLocation, FVector_NetQuantizeNormal ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration)
{
    HandleFireFromClientAim(ClientMuzzleLocation, ClientCameraLocation, FVector(ClientAimDirection).GetSafeNormal(), DamageEffectClass, bDrawDebugShot, DebugShotDuration);
}

void UWeaponComponent::ClientCorrectMagazineAmmo_Implementation(const int32 ServerMagazineAmmo)
{
    SetCurrentMagazineAmmo(ServerMagazineAmmo);
}

bool UWeaponComponent::GetLeftHandIKTransform(USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const
{
    return GetLeftHandIKTransformForTarget(LeftHandIKSocketName, CharacterMesh, RelativeToBoneName, OutTransform);
}

bool UWeaponComponent::GetLeftHandIKTransformForTarget(FName TargetName, USkeletalMeshComponent* CharacterMesh, FName RelativeToBoneName, FTransform& OutTransform) const
{
    if (!CurrentWeapon)
    {
        OutTransform = FTransform::Identity;
        return false;
    }

    return CurrentWeapon->GetSocketTransformInBoneSpace(TargetName, CharacterMesh, RelativeToBoneName, OutTransform);
}

bool UWeaponComponent::GetLeftHandIKJointTargetLocation(
    USkeletalMeshComponent* CharacterMesh,
    FVector& OutLocation) const
{
    OutLocation = FVector::ZeroVector;

    if (!CharacterMesh || LeftHandIKJointRootBoneName.IsNone()
        || !CharacterMesh->DoesSocketExist(LeftHandIKJointRootBoneName))
    {
        return false;
    }

    const FVector RootLocation = CharacterMesh->GetSocketTransform(
        LeftHandIKJointRootBoneName,
        RTS_Component).GetLocation();
    OutLocation = RootLocation + LeftHandIKJointTargetOffset;
    return true;
}

void UWeaponComponent::TestEquipWeapon()
{
    Server_TestEquipWeapon();
}

void UWeaponComponent::Server_TestEquipWeapon_Implementation()
{
    if (!TestPickupClass) return;

    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return;

    GetWorld()->SpawnActor<AWeaponPickupActor>(TestPickupClass, Owner->GetActorLocation(), FRotator::ZeroRotator);
}

void UWeaponComponent::EquipWeapon(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass)
{
    UE_LOG(LogTemp, Warning, TEXT("EquipWeapon called: Owner=%s Mesh=%s Type=%d WeaponActorClass=%s Authority=%s"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
        NewMesh ? *NewMesh->GetName() : TEXT("None"),
        static_cast<int32>(NewType),
        NewWeaponActorClass ? *NewWeaponActorClass->GetName() : TEXT("None"),
        GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));

    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    ApplyEquip(NewMesh, NewType, NewAnimLayer, NewWeaponActorClass);
}

void UWeaponComponent::EquipWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !NewDefinition)
    {
        return;
    }

    ApplyWeaponDefinition(NewDefinition);

    // 픽업으로 무기를 주우면 지금 든 슬롯의 내용이 바뀐 것이다.
    // 슬롯을 갱신하지 않으면 1·2 키로 돌아왔을 때 주운 무기가 사라져 표시와 실제가 어긋난다.
    if (WeaponSlots.IsValidIndex(ActiveWeaponSlot))
    {
        FLastFPSWeaponSlotState& ActiveSlot = WeaponSlots[ActiveWeaponSlot];
        ActiveSlot.Definition = NewDefinition;
        // 새 무기는 방금 가득 채워졌으므로 이전 무기의 보관 탄약은 버린다.
        ActiveSlot.StashedMagazineAmmo = CurrentMagazineAmmo;
        ActiveSlot.StashedReserveAmmo = CurrentReserveAmmo;
        ActiveSlot.bAmmoInitialized = HasWeapon();
        OnWeaponLoadoutChanged.Broadcast();
    }
}

void UWeaponComponent::UnequipWeapon()
{
    if (!GetOwner())
    {
        return;
    }

    ResetPendingAimRecoil();
    ResetAimRecoilSequence();

    if (!GetOwner()->HasAuthority())
    {
        Server_UnequipWeapon();
        return;
    }

    ClearCurrentWeaponState();

    // 명시적인 전체 해제는 슬롯 구성까지 제거한다.
    if (WeaponSlots.Num() > 0)
    {
        WeaponSlots.Reset();
        ActiveWeaponSlot = DefaultBattleWeaponSlotIndex;
        OnWeaponLoadoutChanged.Broadcast();
        OnWeaponSlotChanged.Broadcast(ActiveWeaponSlot);
    }
}

void UWeaponComponent::ClearCurrentWeaponState()
{
    WeaponDefinition = nullptr;
    WeaponSkeletalMesh = nullptr;
    WeaponType = EMMWeaponType::Unarmed;
    WeaponAnimLayerClass = nullptr;
    DestroyCurrentWeapon();
    SetCurrentMagazineAmmo(0);
    SetCurrentReserveAmmo(0);

    ApplyAnimLayerClass(UnarmedAnimLayerClass);
    OnWeaponEquippedChanged.Broadcast(false);
}

void UWeaponComponent::Server_UnequipWeapon_Implementation()
{
    UnequipWeapon();
}

ULastFPSWeaponDefinition* UWeaponComponent::GetWeaponDefinitionForSlot(const int32 SlotIndex) const
{
    return WeaponSlots.IsValidIndex(SlotIndex) ? WeaponSlots[SlotIndex].Definition : nullptr;
}

void UWeaponComponent::SetWeaponLoadout(const TArray<ULastFPSWeaponDefinition*>& SlotDefinitions)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    WeaponSlots.Reset(SlotDefinitions.Num());
    for (ULastFPSWeaponDefinition* Definition : SlotDefinitions)
    {
        FLastFPSWeaponSlotState& Slot = WeaponSlots.AddDefaulted_GetRef();
        Slot.Definition = Definition;
    }

    OnWeaponLoadoutChanged.Broadcast();

    // 배틀 진입 시에는 HUD의 1번 키와 대응하는 첫 번째 슬롯을 기본 활성 슬롯으로 삼는다.
    // 해당 슬롯이 비어 있으면 다른 슬롯을 임의로 선택하지 않고 2번 입력을 기다린다.
    ActiveWeaponSlot = DefaultBattleWeaponSlotIndex;
    if (!WeaponSlots.IsValidIndex(ActiveWeaponSlot) || !WeaponSlots[ActiveWeaponSlot].Definition)
    {
        ResetPendingAimRecoil();
        ResetAimRecoilSequence();
        // 빈 로드아웃도 슬롯 개수는 UI와 이후 픽업 장착 계약으로 유지한다.
        ClearCurrentWeaponState();
        OnWeaponSlotChanged.Broadcast(ActiveWeaponSlot);
        return;
    }

    // 이전 로드아웃의 탄약이 새 구성으로 새지 않도록 보관값을 거치지 않고 곧바로 장착한다.
    ApplyWeaponDefinition(WeaponSlots[ActiveWeaponSlot].Definition);
    WeaponSlots[ActiveWeaponSlot].bAmmoInitialized = HasWeapon();
    OnWeaponSlotChanged.Broadcast(ActiveWeaponSlot);
}

void UWeaponComponent::RequestWeaponSlot(const int32 SlotIndex)
{
    if (!GetOwner())
    {
        return;
    }

    if (!GetOwner()->HasAuthority())
    {
        Server_RequestWeaponSlot(SlotIndex);
        return;
    }

    ApplyWeaponSlot(SlotIndex);
}

void UWeaponComponent::Server_RequestWeaponSlot_Implementation(const int32 SlotIndex)
{
    // 클라이언트가 보낸 인덱스를 그대로 믿지 않고 서버 상태로 검증한다(ApplyWeaponSlot 내부).
    ApplyWeaponSlot(SlotIndex);
}

void UWeaponComponent::StashActiveSlotAmmo()
{
    if (!WeaponSlots.IsValidIndex(ActiveWeaponSlot))
    {
        return;
    }

    FLastFPSWeaponSlotState& Slot = WeaponSlots[ActiveWeaponSlot];
    if (!Slot.Definition)
    {
        return;
    }

    Slot.StashedMagazineAmmo = CurrentMagazineAmmo;
    Slot.StashedReserveAmmo = CurrentReserveAmmo;
    Slot.bAmmoInitialized = true;
}

void UWeaponComponent::ApplyWeaponSlot(const int32 SlotIndex)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    if (!WeaponSlots.IsValidIndex(SlotIndex) || SlotIndex == ActiveWeaponSlot)
    {
        return;
    }

    // 빈 슬롯으로는 전환하지 않는다. 맨손 전환은 UnequipWeapon 이 담당하는 별개의 동작이다.
    if (!WeaponSlots[SlotIndex].Definition)
    {
        return;
    }

    StashActiveSlotAmmo();

    ActiveWeaponSlot = SlotIndex;
    FLastFPSWeaponSlotState& NewSlot = WeaponSlots[SlotIndex];

    // ApplyWeaponDefinition 은 밸런스 기준으로 탄약을 가득 채운다.
    // 이미 사용한 적이 있는 슬롯이면 그 위에 보관해 둔 값을 덮어써 소모 상태를 이어간다.
    ApplyWeaponDefinition(NewSlot.Definition);

    // 밸런스 행 누락 등으로 무기가 실제로 스폰되지 않았다면 보관 탄약을 되살리지 않는다.
    // 들지도 않은 무기의 탄약이 HUD에 남는 것을 막는다.
    if (HasWeapon())
    {
        if (NewSlot.bAmmoInitialized)
        {
            SetCurrentMagazineAmmo(NewSlot.StashedMagazineAmmo);
            SetCurrentReserveAmmo(NewSlot.StashedReserveAmmo);
        }
        else
        {
            NewSlot.bAmmoInitialized = true;
        }
    }

    GetOwner()->ForceNetUpdate();
    OnWeaponSlotChanged.Broadcast(ActiveWeaponSlot);
}

void UWeaponComponent::OnRep_WeaponSlots()
{
    OnWeaponLoadoutChanged.Broadcast();
}

void UWeaponComponent::OnRep_ActiveWeaponSlot()
{
    OnWeaponSlotChanged.Broadcast(ActiveWeaponSlot);
}

void UWeaponComponent::ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition)
{
    ResetPendingAimRecoil();
    ResetAimRecoilSequence();
    WeaponDefinition = NewDefinition;
    const bool bHasRequiredWeaponData = ApplyWeaponDefinitionValues(NewDefinition);

    DestroyCurrentWeapon();

    if (!bHasRequiredWeaponData || !WeaponSkeletalMesh)
    {
        SetCurrentMagazineAmmo(0);
        SetCurrentReserveAmmo(0);
        ApplyAnimLayerClass(UnarmedAnimLayerClass);
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(WeaponSkeletalMesh, WeaponActorClass, NewDefinition);
    SetCurrentMagazineAmmo(CurrentWeapon ? MagazineCapacity : 0);
    SetCurrentReserveAmmo(CurrentWeapon ? StartingReserveAmmo : 0);
    NextAllowedServerFireTimeSeconds = 0.0;
    ApplyWeaponVisibilityOverride();
    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

bool UWeaponComponent::ApplyWeaponDefinitionValues(const ULastFPSWeaponDefinition* NewDefinition)
{
    MagazineCapacity = 30;
    StartingReserveAmmo = 90;
    ReloadDuration = 2.f;
    // 밸런스 조회 실패 시 직전에 장착했던 무기의 수치가 남지 않도록 비활성 상태로 초기화한다.
    FireRate = MinimumServerFireIntervalSeconds;
    AimTraceRange = 0.f;
    DamageRange.MinDamage = 0.f;
    DamageRange.MaxDamage = 0.f;
    DamageRange.DamageElement = ELastFPSDamageElement::Physical;
    // 밸런스 행이 없으면 단일탄으로 되돌려, 데이터를 지정하지 않은 무기가 샷건 설정을 물려받지 않게 한다.
    PelletsPerShot = 1;
    SpreadHalfAngleDegrees = 0.f;

    if (!NewDefinition)
    {
        return false;
    }

    WeaponSkeletalMesh = NewDefinition->SkeletalMesh;
    WeaponType = NewDefinition->WeaponType;
    WeaponAnimLayerClass = NewDefinition->AnimLayerClass;
    WeaponActorClass = NewDefinition->WeaponActorClass;
    ProjectileClass = NewDefinition->ProjectileClass;
    MuzzleSocketName = NewDefinition->MuzzleSocketName;
    AttachSocketName = NewDefinition->AttachSocketName;
    LeftHandIKSocketName = NewDefinition->LeftHandIKSocketName;
    ReadyLeftHandIKSocketName = NewDefinition->ReadyLeftHandIKSocketName;
    LeftHandIKJointRootBoneName = NewDefinition->LeftHandIKJointRootBoneName;
    LeftHandIKJointTargetOffset = NewDefinition->LeftHandIKJointTargetOffset;
    ReloadLeftHandIKTargetName = NewDefinition->ReloadLeftHandIKTargetName;

    const UGameInstance* GameInstance = GetOwner() ? GetOwner()->GetGameInstance() : nullptr;
    const ULastFPSWeaponDataSubsystem* WeaponDataSubsystem =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSWeaponDataSubsystem>() : nullptr;
    const FLastFPSWeaponBalanceData* BalanceData =
        WeaponDataSubsystem ? WeaponDataSubsystem->FindBalance(NewDefinition->WeaponId) : nullptr;
    if (!BalanceData)
    {
        UE_LOG(LogTemp, Error,
            TEXT("무기 '%s'의 WeaponBalance 행을 찾지 못해 장착을 중단합니다. WeaponDefinition에는 밸런스 fallback이 없습니다."),
            *NewDefinition->WeaponId.ToString());
        return false;
    }

    MagazineCapacity    = FMath::Max(BalanceData->MagazineCapacity, 1);
    StartingReserveAmmo = FMath::Max(BalanceData->StartingReserveAmmo, 0);
    ReloadDuration      = FMath::Max(BalanceData->ReloadDuration, 0.01f);

    FireRate      = FMath::Max(BalanceData->FireInterval, 0.01f);
    AimTraceRange = FMath::Max(BalanceData->AimTraceRange, 0.f);
    DamageRange   = LastFPSDamage::MakeDamageRange(BalanceData->Damage, BalanceData->DamageElement);

    // 산탄 수치는 공통 밸런스 행 구조를 바꾸지 않고 Parameters 맵(태그 키)에서 읽는다.
    // 지정하지 않은 무기는 폴백(1발, 퍼짐 0)으로 기존 단일탄 동작을 유지한다.
    PelletsPerShot = FMath::Max(
        FMath::RoundToInt(BalanceData->GetParameter(LastFPSGameplayTags::Weapon_Parameter_PelletCount, 1.f)),
        1);
    SpreadHalfAngleDegrees = FMath::Max(
        BalanceData->GetParameter(LastFPSGameplayTags::Weapon_Parameter_SpreadHalfAngle, 0.f),
        0.f);
    return true;
}

void UWeaponComponent::OnRep_CurrentWeapon()
{
    ApplyRestoreMagazineVisual();

    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
        ApplyWeaponVisibilityOverride();
    }

    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
    NotifyAmmoChanged();
    OnWeaponReserveAmmoChanged.Broadcast(CurrentReserveAmmo);
}

void UWeaponComponent::OnRep_WeaponType()
{
    // WeaponType 변경 시 애니메이션이 새 분기를 잡도록 재브로드캐스트
    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::OnRep_WeaponDefinition()
{
    ApplyRestoreMagazineVisual();
    ResetPendingAimRecoil();
    ResetAimRecoilSequence();
    ApplyWeaponDefinitionValues(WeaponDefinition);

    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
    }

    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    // 장착 여부가 같아도 무기별 UI 데이터가 바뀌었으므로 HUD 구독자에게 갱신을 알린다.
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
    NotifyAmmoChanged();
    OnWeaponReserveAmmoChanged.Broadcast(CurrentReserveAmmo);
}

void UWeaponComponent::OnRep_CurrentMagazineAmmo()
{
    NotifyAmmoChanged();
}

void UWeaponComponent::OnRep_CurrentReserveAmmo()
{
    OnWeaponReserveAmmoChanged.Broadcast(CurrentReserveAmmo);
}

void UWeaponComponent::AttachWeaponToOwner(ALastFPSWeaponActor* WeaponActor)
{
    if (!WeaponActor) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    USkeletalMeshComponent* OwnerMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
    if (!OwnerMesh) return;

    // 이미 올바른 부모에 붙어있으면 재부착 생략
    if (WeaponActor->GetAttachParentActor() == OwnerCharacter
        && WeaponActor->GetRootComponent()
        && WeaponActor->GetRootComponent()->GetAttachParent() == OwnerMesh
        && WeaponActor->GetRootComponent()->GetAttachSocketName() == AttachSocketName)
    {
        return;
    }

    const bool bSocketExists = OwnerMesh->DoesSocketExist(AttachSocketName);
    WeaponActor->AttachToComponent(
        OwnerMesh,
        FAttachmentTransformRules::SnapToTargetIncludingScale,
        bSocketExists ? AttachSocketName : NAME_None);

    WeaponActor->SetActorRelativeScale3D(FVector::OneVector);
}

void UWeaponComponent::ApplyEquip(USkeletalMesh* NewMesh, EMMWeaponType NewType, TSubclassOf<UAnimInstance> NewAnimLayer, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass)
{
    UE_LOG(LogTemp, Warning, TEXT("ApplyEquip: Owner=%s Mesh=%s Type=%d WeaponActorClass=%s"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
        NewMesh ? *NewMesh->GetName() : TEXT("None"),
        static_cast<int32>(NewType),
        NewWeaponActorClass ? *NewWeaponActorClass->GetName() : TEXT("None"));

    WeaponDefinition = nullptr;
    MagazineCapacity = 30;
    StartingReserveAmmo = 90;
    ReloadDuration = 2.f;
    WeaponSkeletalMesh = NewMesh;
    WeaponType = NewType;
    WeaponAnimLayerClass = NewAnimLayer;
    if (NewWeaponActorClass)
    {
        WeaponActorClass = NewWeaponActorClass;
    }

    DestroyCurrentWeapon();

    if (!NewMesh)
    {
        SetCurrentMagazineAmmo(0);
        SetCurrentReserveAmmo(0);
        ApplyAnimLayerClass(UnarmedAnimLayerClass);
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(NewMesh, NewWeaponActorClass);
    SetCurrentMagazineAmmo(CurrentWeapon ? MagazineCapacity : 0);
    SetCurrentReserveAmmo(CurrentWeapon ? StartingReserveAmmo : 0);
    NextAllowedServerFireTimeSeconds = 0.0;
    ApplyWeaponVisibilityOverride();
    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::ApplyAnimLayerClass(TSubclassOf<UAnimInstance> AnimLayerClass) const
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    USkeletalMeshComponent* OwnerMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
    if (!OwnerMesh || !AnimLayerClass)
    {
        return;
    }

    OwnerMesh->LinkAnimClassLayers(AnimLayerClass);
}

TSubclassOf<UAnimInstance> UWeaponComponent::ResolveCurrentAnimLayerClass() const
{
    if (CurrentWeapon && WeaponAnimLayerClass)
    {
        return WeaponAnimLayerClass;
    }

    return UnarmedAnimLayerClass;
}

void UWeaponComponent::ApplyWeaponVisibilityOverride()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->SetWeaponHidden(WeaponHiddenOverrideCount > 0);
    }
}

ALastFPSWeaponActor* UWeaponComponent::SpawnWeaponActor(USkeletalMesh* NewMesh, TSubclassOf<ALastFPSWeaponActor> NewWeaponActorClass, ULastFPSWeaponDefinition* Definition)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    USkeletalMeshComponent* OwnerMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
    UWorld* World = GetWorld();

    if (!World || !OwnerCharacter || !OwnerMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnWeaponActor failed: World=%s Owner=%s OwnerMesh=%s"),
            World ? TEXT("valid") : TEXT("null"),
            OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("null"),
            OwnerMesh ? *OwnerMesh->GetName() : TEXT("null"));
        return nullptr;
    }

    // 무기 액터는 서버에서만 스폰 (클라는 복제 + OnRep_CurrentWeapon으로 attach)
    if (!OwnerCharacter->HasAuthority())
    {
        return nullptr;
    }

    TSubclassOf<ALastFPSWeaponActor> ClassToSpawn = NewWeaponActorClass;
    if (!ClassToSpawn)
    {
        ClassToSpawn = WeaponActorClass;
    }
    if (!ClassToSpawn)
    {
        ClassToSpawn = ALastFPSWeaponActor::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerCharacter;
    SpawnParams.Instigator = OwnerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ALastFPSWeaponActor* NewWeapon = World->SpawnActor<ALastFPSWeaponActor>(ClassToSpawn, FTransform::Identity, SpawnParams);
    if (!NewWeapon)
    {
        return nullptr;
    }

    NewWeapon->InitializeWeapon(NewMesh, Definition);

    AttachWeaponToOwner(NewWeapon);

    return NewWeapon;
}

void UWeaponComponent::DestroyCurrentWeapon()
{
    ApplyRestoreMagazineVisual();

    if (CurrentWeapon)
    {
        CurrentWeapon->Destroy();
        CurrentWeapon = nullptr;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    TArray<AActor*> AttachedActors;
    OwnerActor->GetAttachedActors(AttachedActors);
    for (AActor* AttachedActor : AttachedActors)
    {
        if (ALastFPSWeaponActor* AttachedWeapon = Cast<ALastFPSWeaponActor>(AttachedActor))
        {
            AttachedWeapon->Destroy();
        }
    }
}

void UWeaponComponent::HandleFireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    UWorld* World = GetWorld();
    if (!World || !Character || !Character->HasAuthority())
    {
        return;
    }

    if (!ProjectileClass)
    {
        ClientCorrectMagazineAmmo(CurrentMagazineAmmo);
        return;
    }

    const FVector AimDirection = ClientAimDirection.GetSafeNormal();
    if (AimDirection.IsNearlyZero())
    {
        ClientCorrectMagazineAmmo(CurrentMagazineAmmo);
        return;
    }

    if (!TryConsumeServerFirePermission())
    {
        ClientCorrectMagazineAmmo(CurrentMagazineAmmo);
        return;
    }

    const FVector MuzzleLocation = ValidateClientMuzzleLocation(ClientMuzzleLocation)
        ? ClientMuzzleLocation
        : GetMuzzleTransform().GetLocation();

    const FVector TraceStart = ResolveValidatedTraceStart(*Character, ClientCameraLocation);

    // 방아쇠 1회 = 탄약 1발 소비(위에서 TryConsumeServerFirePermission으로 이미 처리).
    // 그 1발 안에서 PelletsPerShot개의 산탄을 각각 원뿔 퍼짐 방향으로 발사한다.
    const int32 PelletCount = FMath::Max(PelletsPerShot, 1);
    const float SpreadHalfAngleRad = FMath::DegreesToRadians(FMath::Max(SpreadHalfAngleDegrees, 0.f));

    for (int32 PelletIndex = 0; PelletIndex < PelletCount; ++PelletIndex)
    {
        // 첫 펠릿은 조준 중심으로 곧게 보내 조준 신뢰도를 확보하고, 나머지는 원뿔 안에서 무작위로 퍼뜨린다.
        // 단일탄(PelletCount==1)이거나 퍼짐이 0이면 항상 중심 방향이라 기존 단일탄 동작과 동일하다.
        const FVector PelletDirection = (PelletIndex == 0 || SpreadHalfAngleRad <= 0.f)
            ? AimDirection
            : FMath::VRandCone(AimDirection, SpreadHalfAngleRad);

        FireSinglePelletFromServer(
            *Character,
            TraceStart,
            MuzzleLocation,
            PelletDirection,
            DamageEffectClass,
            bDrawDebugShot,
            DebugShotDuration);
    }
}

void UWeaponComponent::FireSinglePelletFromServer(
    ACharacter& Character,
    const FVector& TraceStart,
    const FVector& MuzzleLocation,
    const FVector& PelletDirection,
    TSubclassOf<UGameplayEffect> DamageEffectClass,
    bool bDrawDebugShot,
    float DebugShotDuration)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector TraceEnd = TraceStart + PelletDirection * AimTraceRange;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), false, &Character);
    QueryParams.AddIgnoredActor(&Character);
    QueryParams.AddIgnoredActor(CurrentWeapon);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
    // Pickup Object Channel은 조회하지 않아 획득 아이템 뒤의 실제 조준 대상을 찾는다.

    const bool bHit = World->LineTraceSingleByObjectType(
        HitResult, TraceStart, TraceEnd, ObjectParams, QueryParams);

    const FVector AimTarget = bHit ? HitResult.ImpactPoint : TraceEnd;
    const FRotator ProjectileRotation = (AimTarget - MuzzleLocation).Rotation();

    if (bDrawDebugShot)
    {
        const float SafeDebugDuration = FMath::Clamp(
            DebugShotDuration,
            0.f,
            MaximumServerDebugShotDurationSeconds);
        DrawDebugLine(World, TraceStart, TraceEnd, FColor::Red, false, SafeDebugDuration, 0, 1.f);
        DrawDebugLine(World, MuzzleLocation, AimTarget, FColor::Green, false, SafeDebugDuration, 0, 2.f);
        DrawDebugSphere(World, MuzzleLocation, 8.f, 12, FColor::Green, false, SafeDebugDuration);
        DrawDebugSphere(World, AimTarget, 8.f, 12, FColor::Red, false, SafeDebugDuration);
    }

    const FTransform ProjectileTransform(ProjectileRotation, MuzzleLocation);
    ALastFPSProjectile* VisualProjectile = nullptr;
    if (ULastFPSActorPoolSubsystem* Pool =
        World->GetSubsystem<ULastFPSActorPoolSubsystem>())
    {
        VisualProjectile = Cast<ALastFPSProjectile>(Pool->AcquireActorByClass(
            ProjectileClass,
            ProjectileTransform,
            &Character,
            &Character));
    }
    if (!VisualProjectile)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Instigator = &Character;
        SpawnParams.Owner = &Character;
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        VisualProjectile = World->SpawnActor<ALastFPSProjectile>(
            ProjectileClass,
            ProjectileTransform,
            SpawnParams);
    }
    AActor* HitActor = HitResult.GetActor();
    if (!bHit || !HitActor || !DamageEffectClass
        || !LastFPSDamage::IsDamageGameplayEffect(DamageEffectClass)
        || LastFPSCombatAffiliation::AreFriendlyActors(&Character, HitActor))
    {
        return;
    }

    IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(&Character);
    IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor);
    if (!SourceASI || !TargetASI)
    {
        return;
    }

    UAbilitySystemComponent* SourceASC = SourceASI->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
    if (!SourceASC || !TargetASC)
    {
        return;
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    Context.AddSourceObject(&Character);
    Context.AddInstigator(&Character, &Character);

    // 데미지는 펠릿 단위로 적용된다. 즉 밸런스 행의 Damage는 "펠릿 1발당" 피해이며,
    // 명중한 펠릿 수에 비례해 총 피해가 누적되는 것이 샷건의 의도된 계약이다.
    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
    if (Spec.IsValid())
    {
        LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), DamageRange);
        TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

        // 약점(예: 머리) 본에 맞았으면 약점 컴포넌트에 알려 발광·약점 체력 단계를 갱신한다(서버 권한).
        ULastFPSWeakpointComponent* Weakpoint = HitActor->FindComponentByClass<ULastFPSWeakpointComponent>();
        // [진단] 임시 로그 — 무기 히트가 약점 컴포넌트/본을 제대로 잡는지 확인. 문제 해결 후 제거.
        UE_LOG(LogTemp, Warning, TEXT("[Weakpoint] 무기히트 Actor=%s Bone=%s HitComp=%s WeakpointComp=%s"),
            *GetNameSafe(HitActor),
            *HitResult.BoneName.ToString(),
            *GetNameSafe(HitResult.GetComponent()),
            Weakpoint ? TEXT("있음") : TEXT("없음"));
        if (Weakpoint)
        {
            Weakpoint->HandleHitOnBone(HitResult.BoneName, GetWeaponBaseDamage());
        }

        // 히트마커는 데미지 적용 중앙 이벤트(LastFPSAttributeSet → PlayerState::OnDamageDealt → HUD)에서
        // 모든 데미지 GA에 대해 일괄 처리한다. 무기 전용 호출은 중복이므로 제거했다.
    }
}

const FLastFPSWeaponScopeSettings* UWeaponComponent::GetScopeSettings() const
{
    if (!WeaponDefinition || !WeaponDefinition->Scope.bUseScope)
    {
        return nullptr;
    }

    return &WeaponDefinition->Scope;
}

bool UWeaponComponent::ValidateClientMuzzleLocation(const FVector& ClientMuzzleLocation) const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return false;
    }

    const FVector ServerWeaponLocation = GetMuzzleTransform().GetLocation();
    const float MaxError = FMath::Max(MaxClientMuzzleLocationError, 0.f);
    return FVector::DistSquared(ClientMuzzleLocation, ServerWeaponLocation) <= FMath::Square(MaxError);
}

FVector UWeaponComponent::ResolveValidatedTraceStart(
    const ACharacter& Character,
    const FVector& ClientCameraLocation) const
{
    FVector ServerViewLocation;
    FRotator ServerViewRotation;
    if (const AController* Controller = Character.GetController())
    {
        Controller->GetPlayerViewPoint(ServerViewLocation, ServerViewRotation);
    }
    else
    {
        Character.GetActorEyesViewPoint(ServerViewLocation, ServerViewRotation);
    }

    const float MaxError = FMath::Max(MaxClientCameraLocationError, 0.f);
    return FVector::DistSquared(ClientCameraLocation, ServerViewLocation) <= FMath::Square(MaxError)
        ? ClientCameraLocation
        : ServerViewLocation;
}

bool UWeaponComponent::TryConsumeServerFirePermission()
{
    const UWorld* World = GetWorld();
    if (!World || !GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (CurrentMagazineAmmo <= 0 || !CurrentWeapon)
    {
        return false;
    }

    const double CurrentTimeSeconds = World->GetTimeSeconds();
    const double ToleranceSeconds = FMath::Max(ServerFireIntervalTolerance, 0.f);
    if (CurrentTimeSeconds + ToleranceSeconds < NextAllowedServerFireTimeSeconds)
    {
        return false;
    }

    NextAllowedServerFireTimeSeconds =
        FMath::Max(CurrentTimeSeconds, NextAllowedServerFireTimeSeconds)
        + FMath::Max(FireRate, MinimumServerFireIntervalSeconds);
    SetCurrentMagazineAmmo(CurrentMagazineAmmo - 1);
    GetOwner()->ForceNetUpdate();
    return true;
}

void UWeaponComponent::SetCurrentMagazineAmmo(const int32 NewMagazineAmmo)
{
    const int32 ClampedAmmo = FMath::Clamp(NewMagazineAmmo, 0, FMath::Max(MagazineCapacity, 0));
    if (CurrentMagazineAmmo == ClampedAmmo)
    {
        return;
    }

    CurrentMagazineAmmo = ClampedAmmo;
    NotifyAmmoChanged();
}

int32 UWeaponComponent::Auth_AddReserveAmmo(const int32 Amount)
{
    const AActor* Owner = GetOwner();
    if (Amount <= 0 || !Owner || !Owner->HasAuthority() || !HasWeapon())
    {
        return 0;
    }

    const int32 AddedAmmo = FMath::Min(Amount, FMath::Max(0, StartingReserveAmmo - CurrentReserveAmmo));
    if (AddedAmmo > 0)
    {
        SetCurrentReserveAmmo(CurrentReserveAmmo + AddedAmmo);
    }

    return AddedAmmo;
}

void UWeaponComponent::SetCurrentReserveAmmo(const int32 NewReserveAmmo)
{
    const int32 ClampedAmmo = FMath::Max(NewReserveAmmo, 0);
    if (CurrentReserveAmmo == ClampedAmmo)
    {
        return;
    }

    CurrentReserveAmmo = ClampedAmmo;
    OnWeaponReserveAmmoChanged.Broadcast(CurrentReserveAmmo);
}

void UWeaponComponent::NotifyAmmoChanged()
{
    OnWeaponAmmoChanged.Broadcast(CurrentMagazineAmmo, MagazineCapacity);
}
