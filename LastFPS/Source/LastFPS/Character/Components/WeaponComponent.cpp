#include "Character/Components/WeaponComponent.h"

#include "Net/UnrealNetwork.h"
#include "Character/LastFPSCharacterBase.h"
#include "Weapons/LastFPSWeaponActor.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Data/Tables/LastFPSWeaponBalanceData.h"
#include "Weapons/WeaponPickupActor.h"
#include "Weapons/LastFPSWeaponDataSubsystem.h"
#include "Projectiles/LastFPSProjectile.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    constexpr float AimRecoilCompletionToleranceDegrees = 0.01f;
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

bool UWeaponComponent::CanFire() const
{
    return CurrentWeapon != nullptr;
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

void UWeaponComponent::PlayReloadAnimation() const
{
    if (CurrentWeapon)
    {
        CurrentWeapon->PlayReloadAnimation();
    }
}

void UWeaponComponent::FireFromClientAim(const FVector& ClientMuzzleLocation, const FVector& ClientCameraLocation, const FVector& ClientAimDirection, TSubclassOf<UGameplayEffect> DamageEffectClass, bool bDrawDebugShot, float DebugShotDuration)
{
    if (!CanFire())
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
    if (!CanFire())
    {
        return;
    }

    HandleFireFromClientAim(ClientMuzzleLocation, ClientCameraLocation, FVector(ClientAimDirection).GetSafeNormal(), DamageEffectClass, bDrawDebugShot, DebugShotDuration);
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

    WeaponDefinition = nullptr;
    WeaponSkeletalMesh = nullptr;
    WeaponType = EMMWeaponType::Unarmed;
    WeaponAnimLayerClass = nullptr;
    DestroyCurrentWeapon();
    ApplyAnimLayerClass(UnarmedAnimLayerClass);
    OnWeaponEquippedChanged.Broadcast(false);
}

void UWeaponComponent::Server_UnequipWeapon_Implementation()
{
    UnequipWeapon();
}

void UWeaponComponent::ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition)
{
    ResetPendingAimRecoil();
    ResetAimRecoilSequence();
    WeaponDefinition = NewDefinition;
    ApplyWeaponDefinitionValues(NewDefinition);

    DestroyCurrentWeapon();

    if (!WeaponSkeletalMesh)
    {
        ApplyAnimLayerClass(UnarmedAnimLayerClass);
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(WeaponSkeletalMesh, WeaponActorClass, NewDefinition);
    ApplyWeaponVisibilityOverride();
    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::ApplyWeaponDefinitionValues(const ULastFPSWeaponDefinition* NewDefinition)
{
    if (!NewDefinition)
    {
        return;
    }

    WeaponSkeletalMesh = NewDefinition->SkeletalMesh;
    WeaponType = NewDefinition->WeaponType;
    WeaponAnimLayerClass = NewDefinition->AnimLayerClass;
    WeaponActorClass = NewDefinition->WeaponActorClass;
    ProjectileClass = NewDefinition->ProjectileClass;
    MuzzleSocketName = NewDefinition->MuzzleSocketName;
    AttachSocketName = NewDefinition->AttachSocketName;
    LeftHandIKSocketName = NewDefinition->LeftHandIKSocketName;
    ReloadLeftHandIKTargetName = NewDefinition->ReloadLeftHandIKTargetName;
    FireRate = NewDefinition->FireRate;
    DamageRange = NewDefinition->DamageRange;

    const UGameInstance* GameInstance = GetOwner() ? GetOwner()->GetGameInstance() : nullptr;
    const ULastFPSWeaponDataSubsystem* WeaponDataSubsystem =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSWeaponDataSubsystem>() : nullptr;
    const FLastFPSWeaponBalanceData* BalanceData =
        WeaponDataSubsystem ? WeaponDataSubsystem->FindBalance(NewDefinition->WeaponId) : nullptr;
    if (!BalanceData)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("무기 '%s'의 WeaponBalance 행을 찾지 못해 WeaponDefinition 수치를 사용합니다."),
            *NewDefinition->WeaponId.ToString());
        return;
    }

    FireRate = FMath::Max(BalanceData->FireInterval, 0.01f);
    AimTraceRange = FMath::Max(BalanceData->AimTraceRange, 0.f);
    DamageRange = LastFPSDamage::MakeDamageRange(BalanceData->Damage, BalanceData->DamageElement);
}

void UWeaponComponent::OnRep_CurrentWeapon()
{
    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
        ApplyWeaponVisibilityOverride();
    }

    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::OnRep_WeaponType()
{
    // WeaponType 변경 시 애니메이션이 새 분기를 잡도록 재브로드캐스트
    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::OnRep_WeaponDefinition()
{
    ResetPendingAimRecoil();
    ResetAimRecoilSequence();
    ApplyWeaponDefinitionValues(WeaponDefinition);

    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
    }

    ApplyAnimLayerClass(ResolveCurrentAnimLayerClass());
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
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
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
        ApplyAnimLayerClass(UnarmedAnimLayerClass);
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(NewMesh, NewWeaponActorClass);
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
    if (!World || !Character || !ProjectileClass)
    {
        return;
    }

    const FVector AimDirection = ClientAimDirection.GetSafeNormal();
    if (AimDirection.IsNearlyZero())
    {
        return;
    }

    const FVector MuzzleLocation = ValidateClientMuzzleLocation(ClientMuzzleLocation)
        ? ClientMuzzleLocation
        : GetMuzzleTransform().GetLocation();

    const FVector TraceEnd = ClientCameraLocation + AimDirection * AimTraceRange;

    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponTrace), false, Character);
    QueryParams.AddIgnoredActor(Character);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    const bool bHit = World->LineTraceSingleByObjectType(
        HitResult, ClientCameraLocation, TraceEnd, ObjectParams, QueryParams);

    const FVector AimTarget = bHit ? HitResult.ImpactPoint : TraceEnd;
    const FRotator ProjectileRotation = (AimTarget - MuzzleLocation).Rotation();

    if (bDrawDebugShot)
    {
        DrawDebugLine(World, ClientCameraLocation, TraceEnd, FColor::Red, false, DebugShotDuration, 0, 1.f);
        DrawDebugLine(World, MuzzleLocation, AimTarget, FColor::Green, false, DebugShotDuration, 0, 2.f);
        DrawDebugSphere(World, MuzzleLocation, 8.f, 12, FColor::Green, false, DebugShotDuration);
        DrawDebugSphere(World, AimTarget, 8.f, 12, FColor::Red, false, DebugShotDuration);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Instigator = Character;
    SpawnParams.Owner = Character;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    World->SpawnActor<ALastFPSProjectile>(ProjectileClass, MuzzleLocation, ProjectileRotation, SpawnParams);

    if (!bHit || !HitResult.GetActor() || !DamageEffectClass)
    {
        return;
    }

    IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(Character);
    IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitResult.GetActor());
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
    Context.AddSourceObject(Character);
    Context.AddInstigator(Character, Character);

    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
    if (Spec.IsValid())
    {
        LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), DamageRange);
        TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

        if (ALastFPSCharacterBase* ShooterCharacter = Cast<ALastFPSCharacterBase>(Character))
        {
            ShooterCharacter->Client_NotifyHitMarker();
        }
    }
}

bool UWeaponComponent::ValidateClientMuzzleLocation(const FVector& ClientMuzzleLocation) const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return false;
    }

    const FVector ServerWeaponLocation = GetMuzzleTransform().GetLocation();
    const float MaxAllowedDistance = 150.f;
    return FVector::DistSquared(ClientMuzzleLocation, ServerWeaponLocation) <= FMath::Square(MaxAllowedDistance);
}
