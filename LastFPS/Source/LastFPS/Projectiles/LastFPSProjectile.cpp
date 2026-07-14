#include "Projectiles/LastFPSProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ProjectileRules/LastFPSProjectileImpactRule.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "Data/Projectiles/LastFPSProjectileImpactTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystemComponent.h"
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

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 30000.f;
    ProjectileMovement->MaxSpeed = 30000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->bShouldBounce = false;

    InitialLifeSpan = 1.5f;
}

void ALastFPSProjectile::BeginPlay()
{
    Super::BeginPlay();

    ApplyVisualData();
}

void ALastFPSProjectile::InitializeGameplayProjectile(
    AActor* InSourceActor,
    const TArray<TObjectPtr<ULastFPSProjectileImpactRule>>& InImpactRules,
    const TArray<TSubclassOf<UGameplayEffect>>& InLegacyEffectsOnHit,
    ULastFPSProjectileVisualData* InVisualData,
    const float InBaseDamageOverride)
{
    SourceActor = InSourceActor;
    ImpactRules = InImpactRules;
    LegacyEffectsOnHit = InLegacyEffectsOnHit;
    VisualData = InVisualData;
    BaseDamageOverride = FMath::Max(InBaseDamageOverride, 0.f);
    ApplyVisualData();
    EnableGameplayCollision();
}

void ALastFPSProjectile::EnableGameplayCollision()
{
    if (!CollisionComp)
    {
        return;
    }

    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(LastFPSCollision::PickupObjectChannel, ECR_Ignore);
    CollisionComp->SetGenerateOverlapEvents(true);
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
    if (bHasAppliedHit || !HasAuthority() || !OtherActor || OtherActor == SourceActor || OtherActor == GetOwner())
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
    Destroy();
}

void ALastFPSProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
    if (HasAuthority() && !bHasAppliedHit)
    {
        if (AreFriendlyActors(SourceActor, ImpactResult.GetActor()))
        {
            Destroy();
            return;
        }

        ExecuteImpactRules(ImpactResult.GetActor(), ImpactResult);
        bHasAppliedHit = true;
        PlayImpactFeedback(ImpactResult);
        Destroy();
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

    if (VisualData && VisualData->ImpactNiagaraSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            VisualData->ImpactNiagaraSystem,
            ImpactLocation,
            ImpactRotation,
            VisualData->ImpactEffectScale);
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
