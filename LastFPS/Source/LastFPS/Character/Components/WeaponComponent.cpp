#include "Character/Components/WeaponComponent.h"

#include "Net/UnrealNetwork.h"
#include "Character/LastFPSCharacterBase.h"
#include "Weapons/LastFPSWeaponActor.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Weapons/WeaponPickupActor.h"
#include "Projectiles/LastFPSProjectile.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UWeaponComponent::UWeaponComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
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

    const FTransform MuzzleTransform = CurrentWeapon->GetMuzzleTransform(MuzzleSocketName);
    UE_LOG(LogTemp, Warning, TEXT("MuzzleDebug WeaponActor=%s MuzzleSocket=%s MuzzleLoc=%s MuzzleRot=%s"),
        *CurrentWeapon->GetName(),
        *MuzzleSocketName.ToString(),
        *MuzzleTransform.GetLocation().ToString(),
        *MuzzleTransform.Rotator().ToString());

    return MuzzleTransform;
}

void UWeaponComponent::PlayFireEffects() const
{
    if (!CurrentWeapon || !CurrentWeapon->GetWeaponMesh())
    {
        return;
    }

    USkeletalMeshComponent* CurrentWeaponMesh = CurrentWeapon->GetWeaponMesh();
    CurrentWeapon->PlayFireAnimation();

    if (FireSound)
    {
        UGameplayStatics::SpawnSoundAttached(FireSound, CurrentWeaponMesh, MuzzleSocketName);
    }

    if (MuzzleFlashEffect)
    {
        UGameplayStatics::SpawnEmitterAttached(
            MuzzleFlashEffect,
            CurrentWeaponMesh,
            MuzzleSocketName,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget);
    }
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

void UWeaponComponent::ApplyWeaponDefinition(ULastFPSWeaponDefinition* NewDefinition)
{
    WeaponDefinition = NewDefinition;
    ApplyWeaponDefinitionValues(NewDefinition);

    DestroyCurrentWeapon();

    if (!WeaponSkeletalMesh)
    {
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(WeaponSkeletalMesh, WeaponActorClass, NewDefinition);
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
    FireSound = NewDefinition->FireSound;
    MuzzleFlashEffect = NewDefinition->MuzzleFlashEffect;
}

void UWeaponComponent::OnRep_CurrentWeapon()
{
    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
    }

    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::OnRep_WeaponType()
{
    // WeaponType 변경 시 애니메이션이 새 분기를 잡도록 재브로드캐스트
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
}

void UWeaponComponent::OnRep_WeaponDefinition()
{
    ApplyWeaponDefinitionValues(WeaponDefinition);

    if (CurrentWeapon)
    {
        AttachWeaponToOwner(CurrentWeapon);
    }
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
        OnWeaponEquippedChanged.Broadcast(false);
        return;
    }

    CurrentWeapon = SpawnWeaponActor(NewMesh, NewWeaponActorClass);
    OnWeaponEquippedChanged.Broadcast(CurrentWeapon != nullptr);
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

    NewWeapon->InitializeWeapon(NewMesh, MuzzleFlashEffect, FireSound, Definition);

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

    const FVector TraceEnd = ClientCameraLocation + AimDirection * 10000.f;

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
