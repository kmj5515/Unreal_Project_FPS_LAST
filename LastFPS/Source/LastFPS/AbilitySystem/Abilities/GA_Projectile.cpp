#include "AbilitySystem/Abilities/GA_Projectile.h"

#include "AbilitySystemComponent.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Character/LastFPSHero.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Utility/LastFPSTags.h"
#include "Projectiles/LastFPSProjectile.h"

UGA_Projectile::UGA_Projectile()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    CooldownGameplayEffectClass = ULastFPSGE_Skill1Cooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Skill1);
    Tags.AddTag(LastFPSGameplayTags::Input_Skill1);
    SetAssetTags(Tags);
}

void UGA_Projectile::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (!ProjectileData || !Hero || !Hero->IsAlive() || Hero->GetCombatState() != EMMCombatState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_Projectile activation failed: ProjectileData=%s Hero=%s"),
            *GetNameSafe(ProjectileData),
            *GetNameSafe(Hero));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Hero->SetWantsToSprint(false);
    if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
    {
        FGameplayTagContainer SprintTags;
        SprintTags.AddTag(LastFPSGameplayTags::Input_Sprint);
        ASC->CancelAbilities(&SprintTags);
    }

    Hero->SetCombatState(EMMCombatState::Casting);
    bProjectileSpawned = false;
    
    ProjectileSpawnEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        LastFPSGameplayTags::Event_Montage_ProjectileSpawn,
        nullptr,
        true,
        true);
    if (ProjectileSpawnEventTask)
    {
        ProjectileSpawnEventTask->EventReceived.AddDynamic(this, &UGA_Projectile::OnProjectileSpawnEvent);
        ProjectileSpawnEventTask->ReadyForActivation();
    }

    AbilityEndEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        LastFPSGameplayTags::Event_Montage_AbilityEnd,
        nullptr,
        true,
        true);
    if (AbilityEndEventTask)
    {
        AbilityEndEventTask->EventReceived.AddDynamic(this, &UGA_Projectile::OnAbilityEndEvent);
        AbilityEndEventTask->ReadyForActivation();
    }

    if (ProjectileData->CastMontage && Hero->GetMesh())
    {
        if (UAnimInstance* AnimInstance = Hero->GetMesh()->GetAnimInstance())
        {
            const float PlayedDuration = AnimInstance->Montage_Play(
                ProjectileData->CastMontage,
                ProjectileData->MontagePlayRate);
        }
    }

    if (!GetWorld())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UGA_Projectile::SpawnProjectile()
{
    UE_LOG(LogTemp, Warning, TEXT("GA_Projectile SpawnProjectile entered"));

    if (bProjectileSpawned)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_Projectile spawn skipped: already spawned"));
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    UWorld* World = GetWorld();

    if (!ProjectileData || !Hero || !World || !Hero->HasAuthority() || !ProjectileData->ProjectileClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_Projectile spawn skipped: Hero=%s World=%s Authority=%s ProjectileClass=%s"),
            *GetNameSafe(Hero),
            World ? TEXT("valid") : TEXT("null"),
            Hero && Hero->HasAuthority() ? TEXT("true") : TEXT("false"),
            ProjectileData ? *GetNameSafe(ProjectileData->ProjectileClass) : TEXT("null"));
        return;
    }
    bProjectileSpawned = true;

    const FVector CameraAimDirection = GetCameraAimDirection(Hero);
    FRotator SpawnRotation = CameraAimDirection.Rotation();
    FVector SpawnLocation = Hero->GetActorLocation() + FVector(0.f, 0.f, 60.f);

    if (USkeletalMeshComponent* Mesh = Hero->GetMesh())
    {
        if (!ProjectileData->SpawnSocketName.IsNone() && Mesh->DoesSocketExist(ProjectileData->SpawnSocketName))
        {
            SpawnLocation = Mesh->GetSocketLocation(ProjectileData->SpawnSocketName);
        }
    }
    SpawnLocation += SpawnRotation.RotateVector(ProjectileData->SpawnLocationOffset);

    const FVector AimTarget = GetAimTarget(Hero, CameraAimDirection);
    FVector AimDirection = (AimTarget - SpawnLocation).GetSafeNormal();
    if (AimDirection.IsNearlyZero())
    {
        AimDirection = CameraAimDirection;
    }
    SpawnRotation = AimDirection.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Hero;
    SpawnParams.Instigator = Hero;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ALastFPSProjectile* Projectile = World->SpawnActor<ALastFPSProjectile>(
        ProjectileData->ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams);

    if (Projectile)
    {
        Projectile->InitializeGameplayProjectile(
            Hero,
            ProjectileData->ImpactRules,
            ProjectileData->EffectsOnHit,
            ProjectileData->VisualData);
        if (Projectile->ProjectileMovement)
        {
            Projectile->ProjectileMovement->Velocity = AimDirection * ProjectileData->ProjectileSpeed;
        }
    }
}

FVector UGA_Projectile::GetCameraAimDirection(const ALastFPSHero* Hero) const
{
    if (!Hero)
    {
        return FVector::ForwardVector;
    }

    if (const AController* Controller = Hero->GetController())
    {
        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        return ViewRotation.Vector().GetSafeNormal();
    }

    return Hero->GetActorForwardVector().GetSafeNormal();
}

FVector UGA_Projectile::GetAimTarget(const ALastFPSHero* Hero, const FVector& CameraAimDirection) const
{
    if (!Hero || !ProjectileData)
    {
        return FVector::ZeroVector;
    }

    FVector ViewLocation = Hero->GetActorLocation();
    FRotator ViewRotation = Hero->GetActorRotation();
    if (const AController* Controller = Hero->GetController())
    {
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }

    const FVector TraceDirection = CameraAimDirection.IsNearlyZero()
        ? ViewRotation.Vector().GetSafeNormal()
        : CameraAimDirection.GetSafeNormal();
    const FVector TraceEnd = ViewLocation + TraceDirection * ProjectileData->AimTraceRange;

    UWorld* World = GetWorld();
    if (!World)
    {
        return TraceEnd;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileAimTrace), false, Hero);
    QueryParams.AddIgnoredActor(Hero);

    TArray<AActor*> AttachedActors;
    Hero->GetAttachedActors(AttachedActors);
    QueryParams.AddIgnoredActors(AttachedActors);

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    FHitResult HitResult;
    const bool bHit = World->LineTraceSingleByObjectType(
        HitResult,
        ViewLocation,
        TraceEnd,
        ObjectParams,
        QueryParams);

    return bHit ? HitResult.ImpactPoint : TraceEnd;
}

void UGA_Projectile::OnProjectileSpawnEvent(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("GA_Projectile received projectile spawn event: Tag=%s"),
        *Payload.EventTag.ToString());
    SpawnProjectile();
}

void UGA_Projectile::OnAbilityEndEvent(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("GA_Projectile received ability end event: Tag=%s"),
        *Payload.EventTag.ToString());
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Projectile::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (ProjectileSpawnEventTask)
    {
        ProjectileSpawnEventTask->EndTask();
        ProjectileSpawnEventTask = nullptr;
    }

    if (AbilityEndEventTask)
    {
        AbilityEndEventTask->EndTask();
        AbilityEndEventTask = nullptr;
    }

    if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
    {
        if (Hero->GetCombatState() == EMMCombatState::Casting)
        {
            Hero->SetCombatState(EMMCombatState::Idle);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
