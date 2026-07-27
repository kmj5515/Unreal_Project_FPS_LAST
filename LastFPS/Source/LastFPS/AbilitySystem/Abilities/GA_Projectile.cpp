#include "AbilitySystem/Abilities/GA_Projectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "AbilitySystem/Effects/GE_Skill1Cooldown.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Character/LastFPSHero.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameplayPrediction.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Projectiles/LastFPSProjectileAimUtility.h"
#include "Projectiles/LastFPSProjectileLaunchUtility.h"
#include "Utility/LastFPSTags.h"

UGA_Projectile::UGA_Projectile()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    CooldownGameplayEffectClass = ULastFPSGE_Skill1Cooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(LastFPSGameplayTags::Ability_Skill1);
    Tags.AddTag(LastFPSGameplayTags::Input_Skill1);
    SetAssetTags(Tags);

    ActivationBlockedTags.AddTag(LastFPSGameplayTags::State_Combat_Disabled);
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
    bProjectileSpawnEventReceived = false;
    bAimDataSubmitted = false;
    bHasCachedAimTarget = false;
    CachedAimTarget = FVector::ZeroVector;
    RegisterReplicatedAimCallback();
    
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
    UE_LOG(LogTemp, VeryVerbose, TEXT("GA_Projectile SpawnProjectile entered"));

    if (bProjectileSpawned)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_Projectile spawn skipped: already spawned"));
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    UWorld* World = GetWorld();

    if (!ProjectileData || !Hero || !World || !Hero->HasAuthority()
        || !ProjectileData->ProjectileClass || !bHasCachedAimTarget)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GA_Projectile spawn skipped: Hero=%s World=%s Authority=%s ProjectileClass=%s Aim=%s"),
            *GetNameSafe(Hero),
            World ? TEXT("valid") : TEXT("null"),
            Hero && Hero->HasAuthority() ? TEXT("true") : TEXT("false"),
            ProjectileData ? *GetNameSafe(ProjectileData->ProjectileClass) : TEXT("null"),
            bHasCachedAimTarget ? TEXT("valid") : TEXT("pending"));
        return;
    }
    bProjectileSpawned = true;

    FLastFPSProjectileLaunchRequest LaunchRequest;
    LaunchRequest.SourceActor = Hero;
    LaunchRequest.ProjectileData = ProjectileData;
    LaunchRequest.AimTarget = CachedAimTarget;
    LaunchRequest.FallbackAimDirection =
        (CachedAimTarget - Hero->GetActorLocation()).GetSafeNormal();
    const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
    LaunchRequest.BaseDamageOverride = BalanceData && BalanceData->Damage > 0.f
        ? BalanceData->Damage + GetEquippedWeaponBaseDamage()
        : 0.f;
    LastFPSProjectileLaunch::SpawnProjectile(LaunchRequest);
}

void UGA_Projectile::OnProjectileSpawnEvent(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("GA_Projectile received projectile spawn event: Tag=%s"),
        *Payload.EventTag.ToString());
    CaptureAndSubmitLocalAim();

    const ALastFPSHero* Hero =
        Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    if (Hero && Hero->HasAuthority())
    {
        bProjectileSpawnEventReceived = true;
        TrySpawnProjectile();
    }
}

void UGA_Projectile::CaptureAndSubmitLocalAim()
{
    if (bAimDataSubmitted)
    {
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!Hero || !ActorInfo || !ActorInfo->IsLocallyControlled())
    {
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    if (!LastFPSProjectileAim::GetAimViewPoint(
        Hero,
        ViewLocation,
        ViewRotation))
    {
        return;
    }

    const FVector AimDirection = ViewRotation.Vector().GetSafeNormal();
    CacheAimFromView(*Hero, ViewLocation, AimDirection);
    bAimDataSubmitted = true;

    if (Hero->HasAuthority())
    {
        return;
    }

    UAbilitySystemComponent* ASC =
        ActorInfo->AbilitySystemComponent.Get();
    if (!ASC)
    {
        bHasCachedAimTarget = false;
        return;
    }

    FGameplayAbilityTargetData_LocationInfo* AimData =
        new FGameplayAbilityTargetData_LocationInfo();
    AimData->SourceLocation.LocationType =
        EGameplayAbilityTargetingLocationType::LiteralTransform;
    AimData->SourceLocation.LiteralTransform =
        FTransform(ViewRotation, ViewLocation);
    AimData->TargetLocation.LocationType =
        EGameplayAbilityTargetingLocationType::LiteralTransform;
    AimData->TargetLocation.LiteralTransform =
        FTransform(AimDirection.Rotation(), CachedAimTarget);

    FGameplayAbilityTargetDataHandle TargetData(AimData);
    FScopedPredictionWindow PredictionWindow(ASC, true);
    ASC->CallServerSetReplicatedTargetData(
        GetCurrentAbilitySpecHandle(),
        GetCurrentActivationInfo().GetActivationPredictionKey(),
        TargetData,
        FGameplayTag(),
        ASC->ScopedPredictionKey);
}

void UGA_Projectile::RegisterReplicatedAimCallback()
{
    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    UAbilitySystemComponent* ASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent.Get()
        : nullptr;
    if (!Hero || !Hero->HasAuthority() || !ActorInfo
        || ActorInfo->IsLocallyControlled() || !ASC)
    {
        return;
    }

    const FPredictionKey PredictionKey =
        GetCurrentActivationInfo().GetActivationPredictionKey();
    ReplicatedAimDataHandle = ASC->AbilityTargetDataSetDelegate(
        GetCurrentAbilitySpecHandle(),
        PredictionKey).AddUObject(
            this,
            &ThisClass::HandleReplicatedAimData);
    ASC->CallReplicatedTargetDataDelegatesIfSet(
        GetCurrentAbilitySpecHandle(),
        PredictionKey);
}

void UGA_Projectile::UnregisterReplicatedAimCallback()
{
    if (!ReplicatedAimDataHandle.IsValid())
    {
        return;
    }

    if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
    {
        if (UAbilitySystemComponent* ASC =
            ActorInfo->AbilitySystemComponent.Get())
        {
            ASC->AbilityTargetDataSetDelegate(
                GetCurrentAbilitySpecHandle(),
                GetCurrentActivationInfo().GetActivationPredictionKey())
                .Remove(ReplicatedAimDataHandle);
        }
    }
    ReplicatedAimDataHandle.Reset();
}

void UGA_Projectile::CacheAimFromView(
    ALastFPSHero& Hero,
    const FVector& ViewLocation,
    const FVector& AimDirection)
{
    const FVector NormalizedAimDirection = AimDirection.GetSafeNormal();
    const float TraceRange = GetEffectiveAimTraceRange();
    if (NormalizedAimDirection.IsNearlyZero()
        || TraceRange <= KINDA_SMALL_NUMBER)
    {
        bHasCachedAimTarget = false;
        CachedAimTarget = FVector::ZeroVector;
        return;
    }

    CachedAimTarget = LastFPSProjectileAim::GetAimTargetFromView(
        GetWorld(),
        &Hero,
        ViewLocation,
        NormalizedAimDirection,
        TraceRange);
    bHasCachedAimTarget = !CachedAimTarget.ContainsNaN();
}

float UGA_Projectile::GetEffectiveAimTraceRange() const
{
    const FLastFPSSkillBalanceData* BalanceData = GetSkillBalanceData();
    return BalanceData && BalanceData->Range > 0.f
        ? BalanceData->Range
        : ProjectileData
            ? FMath::Max(ProjectileData->AimTraceRange, 0.f)
            : 0.f;
}

void UGA_Projectile::TrySpawnProjectile()
{
    if (bProjectileSpawnEventReceived
        && bHasCachedAimTarget
        && !bProjectileSpawned)
    {
        SpawnProjectile();
    }
}

void UGA_Projectile::HandleReplicatedAimData(
    const FGameplayAbilityTargetDataHandle& Data,
    FGameplayTag /*ActivationTag*/)
{
    // ConsumeClientReplicatedTargetData가 ASC 내부 저장소를 비우므로 먼저 공유 핸들을 보존한다.
    const FGameplayAbilityTargetDataHandle AimDataCopy = Data;
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    UAbilitySystemComponent* ASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent.Get()
        : nullptr;
    if (ASC)
    {
        ASC->ConsumeClientReplicatedTargetData(
            GetCurrentAbilitySpecHandle(),
            GetCurrentActivationInfo().GetActivationPredictionKey());
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo());
    const FGameplayAbilityTargetData* AimData = AimDataCopy.Get(0);
    if (!Hero || !Hero->HasAuthority() || !AimData
        || !AimData->HasOrigin() || !AimData->HasEndPoint())
    {
        return;
    }

    const FVector ClientViewLocation = AimData->GetOrigin().GetLocation();
    const FVector ClientTargetLocation = AimData->GetEndPoint();
    FVector ClientAimDirection =
        (ClientTargetLocation - ClientViewLocation).GetSafeNormal();
    if (ClientAimDirection.IsNearlyZero())
    {
        return;
    }

    FVector ServerViewLocation;
    FRotator ServerViewRotation;
    if (!LastFPSProjectileAim::GetAimViewPoint(
        Hero,
        ServerViewLocation,
        ServerViewRotation))
    {
        return;
    }

    const float CameraLocationTolerance =
        FMath::Max(MaxClientCameraLocationError, 0.f);
    const FVector ValidatedViewLocation =
        FVector::DistSquared(ClientViewLocation, ServerViewLocation)
            <= FMath::Square(CameraLocationTolerance)
        ? ClientViewLocation
        : ServerViewLocation;

    const FVector ServerAimDirection =
        ServerViewRotation.Vector().GetSafeNormal();
    const float AimDot = FVector::DotProduct(
        ClientAimDirection,
        ServerAimDirection);
    const float MinimumAimDot = FMath::Cos(FMath::DegreesToRadians(
        FMath::Clamp(MaxClientAimAngleErrorDegrees, 0.f, 180.f)));
    if (AimDot < MinimumAimDot)
    {
        ClientAimDirection = ServerAimDirection;
    }

    CacheAimFromView(
        *Hero,
        ValidatedViewLocation,
        ClientAimDirection);
    TrySpawnProjectile();
}

void UGA_Projectile::OnAbilityEndEvent(FGameplayEventData Payload)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("GA_Projectile received ability end event: Tag=%s"),
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
    UnregisterReplicatedAimCallback();

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

    bProjectileSpawnEventReceived = false;
    bAimDataSubmitted = false;
    bHasCachedAimTarget = false;
    CachedAimTarget = FVector::ZeroVector;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
