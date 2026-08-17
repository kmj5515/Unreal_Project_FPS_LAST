#include "Projectiles/LastFPSProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "AbilitySystemInterface.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Data/Projectiles/LastFPSProjectileImpactTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "Data/Projectiles/LastFPSProjectileVisualData.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Utility/LastFPSCollisionChannels.h"

namespace
{
	// 투사체 충돌과 효과 적용이 동일한 팀 판정 계약을 사용하도록 진입점을 통일한다.
	bool AreFriendlyActors(const AActor* Source, const AActor* Target)
	{
		return LastFPSCombatAffiliation::AreFriendlyActors(Source, Target);
	}

	bool IsSourceRelatedActor(const AActor* Candidate, const AActor* Source, const AActor* ProjectileOwner)
	{
		return Candidate
			&& (Candidate == Source
				|| Candidate == ProjectileOwner
				|| (Source && Candidate->IsOwnedBy(Source))
				|| (ProjectileOwner && Candidate->IsOwnedBy(ProjectileOwner)));
	}
}

ALastFPSProjectile::ALastFPSProjectile()
{
    bReplicates = true;

    CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
    CollisionComp->InitBoxExtent(FVector(2.5f, 1.f, 1.f));
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionComp->SetGenerateOverlapEvents(false);
    RootComponent = CollisionComp;

    TrailParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticle"));
    TrailParticle->SetupAttachment(CollisionComp);
    TrailParticle->bAutoActivate = true;

    TrailNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagara"));
    TrailNiagara->SetupAttachment(CollisionComp);
    TrailNiagara->bAutoActivate = false;

    FlightAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightAudio"));
    FlightAudio->SetupAttachment(CollisionComp);
    FlightAudio->bAutoActivate = false;
    FlightAudio->bAllowSpatialization = true;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 30000.f;
    ProjectileMovement->MaxSpeed = 30000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->bShouldBounce = false;

    InitialLifeSpan = 0.f;
}

void ALastFPSProjectile::BeginPlay()
{
    Super::BeginPlay();

    ApplyVisualData();
    SetGameplayLifeSpan(DefaultGameplayLifeSpan);
}

void ALastFPSProjectile::InitializeGameplayProjectile(
    AActor* InSourceActor,
    const TArray<TObjectPtr<ULastFPSProjectileImpactRule>>& InImpactRules,
    const TArray<TSubclassOf<UGameplayEffect>>& InLegacyEffectsOnHit,
    ULastFPSProjectileVisualData* InVisualData,
    const float InBaseDamageOverride,
    const FLastFPSProjectileCollisionSettings* InCollisionSettings,
    const FLastFPSProjectileAudioSettings* InAudioSettings)
{
    SourceActor = InSourceActor;
    ImpactRules = InImpactRules;
    LegacyEffectsOnHit = InLegacyEffectsOnHit;
    BaseDamageOverride = FMath::Max(InBaseDamageOverride, 0.f);

    if (HasAuthority() && GetIsReplicated())
    {
        Multicast_ApplyVisualData(InVisualData);
    }
    else
    {
        SetVisualDataLocal(InVisualData);
    }

    ApplyFlightAudio(InAudioSettings);
    EnableGameplayCollision(InCollisionSettings);
    SetGameplayLifeSpan(DefaultGameplayLifeSpan);
}

void ALastFPSProjectile::SetGameplayLifeSpan(const float LifeSeconds)
{
    GetWorldTimerManager().ClearTimer(GameplayLifeTimerHandle);
    if (LifeSeconds > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            GameplayLifeTimerHandle,
            this,
            &ThisClass::FinishProjectile,
            LifeSeconds,
            false);
    }
}

void ALastFPSProjectile::PrepareRenderWarmup(
    ULastFPSProjectileVisualData* InVisualData)
{
    GetWorldTimerManager().ClearTimer(GameplayLifeTimerHandle);
    SetActorEnableCollision(false);
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Deactivate();
    }

    VisualData = InVisualData;
    ApplyVisualData();
    AddRenderWarmupEffectComponents();

    if (TrailNiagara)
    {
        TrailNiagara->PrecachePSOs();
    }
    if (TrailParticle)
    {
        TrailParticle->PrecachePSOs();
    }
}

void ALastFPSProjectile::EnableGameplayCollision(const FLastFPSProjectileCollisionSettings* CollisionSettings)
{
    if (!CollisionComp)
    {
        return;
    }

    const FVector BoxExtent = CollisionSettings
        ? CollisionSettings->BoxExtent.ComponentMax(FVector(0.1f))
        : FVector(2.5f, 1.f, 1.f);
    CollisionComp->SetBoxExtent(BoxExtent, false);

    const bool bUseCollisionProfile = CollisionSettings
        && CollisionSettings->bUseCollisionProfile
        && !CollisionSettings->CollisionProfile.Name.IsNone();
    if (bUseCollisionProfile)
    {
        CollisionComp->SetCollisionProfileName(CollisionSettings->CollisionProfile.Name);
    }
    else
    {
        // 기존 데이터 에셋은 별도 마이그레이션 없이 종전 충돌 규칙을 유지한다.
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
        CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        CollisionComp->SetCollisionResponseToChannel(LastFPSCollision::PickupObjectChannel, ECR_Ignore);
    }

    CollisionComp->SetGenerateOverlapEvents(
        !CollisionSettings || CollisionSettings->bGenerateOverlapEvents);
    CollisionComp->IgnoreActorWhenMoving(SourceActor, true);

    // 무기처럼 별도 Actor로 부착된 부품도 발사자의 일부이므로 이동 충돌에서 제외한다.
    if (SourceActor)
    {
        TArray<AActor*> AttachedActors;
        SourceActor->GetAttachedActors(AttachedActors, true, true);
        for (AActor* AttachedActor : AttachedActors)
        {
            if (AttachedActor)
            {
                CollisionComp->IgnoreActorWhenMoving(AttachedActor, true);
            }
        }
    }

    if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor != SourceActor)
    {
        CollisionComp->IgnoreActorWhenMoving(OwnerActor, true);
    }
    CollisionComp->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALastFPSProjectile::OnProjectileOverlap);

    if (ProjectileMovement)
    {
        ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ALastFPSProjectile::OnProjectileStop);
    }
}

void ALastFPSProjectile::OnProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (bHasAppliedHit || !HasAuthority() || !OtherActor
        || IsSourceRelatedActor(OtherActor, SourceActor, GetOwner()))
    {
        return;
    }

    // 아군은 데미지 없이 통과하며 히트 처리나 투사체 파괴를 수행하지 않는다.
    if (AreFriendlyActors(SourceActor, OtherActor))
    {
        return;
    }

    ExecuteImpactRules(OtherActor, SweepResult);
    bHasAppliedHit = true;
    PlayImpactFeedback(SweepResult);
    FinishProjectile();
}

void ALastFPSProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
    if (HasAuthority() && !bHasAppliedHit)
    {
        AActor* HitActor = ImpactResult.GetActor();
        if (IsSourceRelatedActor(HitActor, SourceActor, GetOwner())
            || AreFriendlyActors(SourceActor, HitActor))
        {
            FinishProjectile();
            return;
        }

        ExecuteImpactRules(HitActor, ImpactResult);
        bHasAppliedHit = true;
        PlayImpactFeedback(ImpactResult);
        FinishProjectile();
    }
}

void ALastFPSProjectile::ExecuteImpactRules(AActor* HitActor, const FHitResult& ImpactResult)
{
    IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(SourceActor);
    UAbilitySystemComponent* SourceASC = SourceASI ? SourceASI->GetAbilitySystemComponent() : nullptr;
    if (!SourceASC)
    {
        return;
    }

    FLastFPSProjectileImpactContext Context;
    Context.SourceActor = SourceActor;
    Context.ProjectileActor = this;
    Context.HitActor = HitActor;
    Context.SourceASC = SourceASC;
    Context.HitResult = ImpactResult;
    Context.BaseDamageOverride = BaseDamageOverride;

    bool bExecutedRule = false;
    for (const TObjectPtr<ULastFPSProjectileImpactRule>& ImpactRule : ImpactRules)
    {
        if (!ImpactRule)
        {
            continue;
        }

        ImpactRule->ExecuteImpact(Context);
        bExecutedRule = true;
    }

    if (bExecutedRule)
    {
        return;
    }

    for (const TSubclassOf<UGameplayEffect>& EffectClass : LegacyEffectsOnHit)
    {
        ApplyEffectToTarget(HitActor, EffectClass);
    }
}

void ALastFPSProjectile::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass)
{
    if (!SourceActor || !TargetActor || !EffectClass
        || AreFriendlyActors(SourceActor, TargetActor))
    {
        return;
    }

    IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(SourceActor);
    IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
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
    Context.AddSourceObject(this);
    Context.AddInstigator(SourceActor, this);

    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
    if (Spec.IsValid())
    {
        const FLastFPSDamageRange EffectiveDamageRange = BaseDamageOverride > 0.f
            ? LastFPSDamage::MakeDamageRange(BaseDamageOverride, LegacyDamageRange.DamageElement)
            : LegacyDamageRange;
        LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), EffectiveDamageRange);
        TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void ALastFPSProjectile::ApplyVisualData()
{
    const FVector TrailEffectScale = VisualData ? VisualData->TrailEffectScale : FVector::OneVector;

    if (TrailNiagara)
    {
        TrailNiagara->SetRelativeScale3D(TrailEffectScale);

        if (VisualData && VisualData->TrailNiagaraSystem)
        {
            TrailNiagara->SetAsset(VisualData->TrailNiagaraSystem);
            TrailNiagara->Activate(true);
        }
        else
        {
            TrailNiagara->Deactivate();
        }
    }

    if (TrailParticle)
    {
        TrailParticle->SetRelativeScale3D(TrailEffectScale);

        if (VisualData && VisualData->TrailNiagaraSystem)
        {
            TrailParticle->DeactivateSystem();
            return;
        }

        if (VisualData && VisualData->TrailEffect)
        {
            TrailParticle->SetTemplate(VisualData->TrailEffect);
            TrailParticle->ActivateSystem(true);
            return;
        }

        if (TrailEffect)
        {
            TrailParticle->SetTemplate(TrailEffect);
            TrailParticle->ActivateSystem(true);
        }
    }
}

void ALastFPSProjectile::SetVisualDataLocal(ULastFPSProjectileVisualData* InVisualData)
{
    VisualData = InVisualData;
    ApplyVisualData();
}

void ALastFPSProjectile::Multicast_ApplyVisualData_Implementation(
    ULastFPSProjectileVisualData* InVisualData)
{
    SetVisualDataLocal(InVisualData);
}

void ALastFPSProjectile::AddRenderWarmupEffectComponents()
{
    ClearRenderWarmupComponents();
    if (!VisualData)
    {
        return;
    }

    if (VisualData->ImpactNiagaraSystem)
    {
        UNiagaraComponent* ImpactNiagara =
            NewObject<UNiagaraComponent>(this);
        ImpactNiagara->bAutoActivate = false;
        ImpactNiagara->SetAsset(VisualData->ImpactNiagaraSystem);
        ImpactNiagara->SetupAttachment(GetRootComponent());
        AddInstanceComponent(ImpactNiagara);
        ImpactNiagara->RegisterComponent();
        ImpactNiagara->PrecachePSOs();
        RenderWarmupComponents.Add(ImpactNiagara);
    }

    if (VisualData->ImpactEffect)
    {
        UParticleSystemComponent* ImpactParticle =
            NewObject<UParticleSystemComponent>(this);
        ImpactParticle->bAutoActivate = false;
        ImpactParticle->SetTemplate(VisualData->ImpactEffect);
        ImpactParticle->SetupAttachment(GetRootComponent());
        AddInstanceComponent(ImpactParticle);
        ImpactParticle->RegisterComponent();
        ImpactParticle->PrecachePSOs();
        RenderWarmupComponents.Add(ImpactParticle);
    }
}

void ALastFPSProjectile::ClearRenderWarmupComponents()
{
    for (UActorComponent* Component : RenderWarmupComponents)
    {
        if (IsValid(Component))
        {
            RemoveInstanceComponent(Component);
            Component->DestroyComponent();
        }
    }
    RenderWarmupComponents.Reset();
}

void ALastFPSProjectile::ApplyFlightAudio(
    const FLastFPSProjectileAudioSettings* AudioSettings)
{
    USoundBase* FlightSound =
        AudioSettings ? AudioSettings->FlightSound.Get() : nullptr;
    const float VolumeMultiplier = AudioSettings
        ? FMath::Max(AudioSettings->VolumeMultiplier, 0.f)
        : 1.f;
    const float PitchMultiplier = AudioSettings
        ? FMath::Max(AudioSettings->PitchMultiplier, 0.01f)
        : 1.f;

    if (HasAuthority() && GetIsReplicated())
    {
        Multicast_ApplyFlightAudio(
            FlightSound,
            VolumeMultiplier,
            PitchMultiplier);
        return;
    }

    SetFlightAudioLocal(
        FlightSound,
        VolumeMultiplier,
        PitchMultiplier);
}

void ALastFPSProjectile::StopFlightAudio()
{
    if (HasAuthority() && GetIsReplicated())
    {
        Multicast_ApplyFlightAudio(nullptr, 1.f, 1.f);
        return;
    }

    SetFlightAudioLocal(nullptr, 1.f, 1.f);
}

void ALastFPSProjectile::StopTrail()
{
    if (HasAuthority() && GetIsReplicated())
    {
        Multicast_StopTrail();
        return;
    }

    StopTrailLocal();
}

void ALastFPSProjectile::Multicast_StopTrail_Implementation()
{
    StopTrailLocal();
}

void ALastFPSProjectile::StopTrailLocal()
{
    if (TrailNiagara)
    {
        TrailNiagara->DeactivateImmediate();
    }
    if (TrailParticle)
    {
        TrailParticle->DeactivateSystem();
    }
}

void ALastFPSProjectile::Multicast_ApplyFlightAudio_Implementation(
    USoundBase* FlightSound,
    const float VolumeMultiplier,
    const float PitchMultiplier)
{
    SetFlightAudioLocal(
        FlightSound,
        VolumeMultiplier,
        PitchMultiplier);
}

void ALastFPSProjectile::SetFlightAudioLocal(
    USoundBase* FlightSound,
    const float VolumeMultiplier,
    const float PitchMultiplier)
{
    if (!FlightAudio)
    {
        return;
    }

    FlightAudio->Stop();
    FlightAudio->SetSound(FlightSound);
    if (!FlightSound)
    {
        return;
    }

    FlightAudio->SetVolumeMultiplier(FMath::Max(VolumeMultiplier, 0.f));
    FlightAudio->SetPitchMultiplier(FMath::Max(PitchMultiplier, 0.01f));
    FlightAudio->Play();
}

void ALastFPSProjectile::PlayImpactFeedback(const FHitResult& ImpactResult)
{
    FVector ImpactLocation = GetActorLocation();
    FRotator ImpactRotation = GetActorRotation();
    if (ImpactResult.bBlockingHit)
    {
        ImpactLocation = ImpactResult.ImpactPoint;
        if (!ImpactResult.ImpactNormal.IsNearlyZero())
        {
            ImpactRotation = ImpactResult.ImpactNormal.Rotation();
        }
    }

    if (HasAuthority() && GetIsReplicated())
    {
        Multicast_PlayImpactFeedback(ImpactLocation, ImpactRotation);
        return;
    }

    PlayImpactFeedbackLocal(ImpactLocation, ImpactRotation);
}

void ALastFPSProjectile::Multicast_PlayImpactFeedback_Implementation(
    const FVector ImpactLocation,
    const FRotator ImpactRotation)
{
    PlayImpactFeedbackLocal(ImpactLocation, ImpactRotation);
}

void ALastFPSProjectile::PlayImpactFeedbackLocal(
    const FVector& ImpactLocation,
    const FRotator& ImpactRotation)
{
    if (VisualData && VisualData->ImpactNiagaraSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            VisualData->ImpactNiagaraSystem,
            ImpactLocation,
            ImpactRotation,
            VisualData->ImpactEffectScale,
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);
    }
    else if (VisualData && VisualData->ImpactEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            this,
            VisualData->ImpactEffect,
            ImpactLocation,
            ImpactRotation,
            VisualData->ImpactEffectScale);
    }

    if (VisualData && VisualData->ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, VisualData->ImpactSound, ImpactLocation);
    }
}

void ALastFPSProjectile::FinishProjectile()
{
    if (ULastFPSActorPoolSubsystem* Pool =
        GetWorld() ? GetWorld()->GetSubsystem<ULastFPSActorPoolSubsystem>() : nullptr)
    {
        if (Pool->ReleaseActor(this))
        {
            return;
        }
    }
    Destroy();
}

void ALastFPSProjectile::OnAcquiredFromPool_Implementation()
{
    ClearRenderWarmupComponents();
    bHasAppliedHit = false;
    SourceActor = nullptr;
    ImpactRules.Reset();
    LegacyEffectsOnHit.Reset();
    VisualData = nullptr;
    BaseDamageOverride = 0.f;
    SetActorEnableCollision(true);

    if (CollisionComp)
    {
        CollisionComp->ClearMoveIgnoreActors();
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CollisionComp->SetGenerateOverlapEvents(false);
    }
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Activate(true);
    }
    ApplyVisualData();
    SetGameplayLifeSpan(DefaultGameplayLifeSpan);
}

void ALastFPSProjectile::OnReleasedToPool_Implementation()
{
    ClearRenderWarmupComponents();
    StopFlightAudio();
    GetWorldTimerManager().ClearTimer(GameplayLifeTimerHandle);
    // 이동 컴포넌트 정지 과정에서 충돌 콜백이 들어와도 반환을 중복 실행하지 않게 잠근다.
    bHasAppliedHit = true;
    SourceActor = nullptr;
    ImpactRules.Reset();
    LegacyEffectsOnHit.Reset();
    VisualData = nullptr;
    BaseDamageOverride = 0.f;

    if (CollisionComp)
    {
        CollisionComp->SetGenerateOverlapEvents(false);
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CollisionComp->ClearMoveIgnoreActors();
    }
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Deactivate();
    }
    StopTrail();
}

void ALastFPSProjectile::OnPrepareForPoolRenderWarmup_Implementation()
{
    ApplyVisualData();
}
