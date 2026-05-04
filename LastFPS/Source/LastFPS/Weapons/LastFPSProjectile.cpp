#include "Weapons/LastFPSProjectile.h"
#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/BoxComponent.h"
#include "GameplayEffectTypes.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"

ALastFPSProjectile::ALastFPSProjectile()
{
    bReplicates = true;

    CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
    CollisionComp->InitBoxExtent(FVector(2.5f, 1.f, 1.f));
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    CollisionComp->OnComponentHit.AddDynamic(this, &ALastFPSProjectile::OnHit);
    RootComponent = CollisionComp;

    TrailParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticle"));
    TrailParticle->SetupAttachment(CollisionComp);
    TrailParticle->bAutoActivate = true;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed          = 12000.f;
    ProjectileMovement->MaxSpeed             = 12000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale   = 0.1f;

    InitialLifeSpan = 3.f;

    // BP에서 덮어쓰지 않으면 네이티브 피해 GE (DamageEffect 미할당 시)
    DamageEffect = ULastFPSGE_DamageInstant::StaticClass();
}

void ALastFPSProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (TrailEffect)
        TrailParticle->SetTemplate(TrailEffect);

    if (GetInstigator())
        CollisionComp->IgnoreActorWhenMoving(GetInstigator(), true);
}

void ALastFPSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                                const FHitResult& Hit)
{
    if (!HasAuthority()) return;
    if (!OtherActor || OtherActor == GetInstigator() || !DamageEffect) { Destroy(); return; }

    IAbilitySystemInterface* InstigatorASI = Cast<IAbilitySystemInterface>(GetInstigator());
    IAbilitySystemInterface* TargetASI     = Cast<IAbilitySystemInterface>(OtherActor);

    if (InstigatorASI && TargetASI)
    {
        UAbilitySystemComponent* SourceASC = InstigatorASI->GetAbilitySystemComponent();
        UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();

        if (SourceASC && TargetASC)
        {
            FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
            Context.AddSourceObject(this);
            if (APawn* Shooter = Cast<APawn>(GetInstigator()))
                Context.AddInstigator(Shooter, this);

            FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
            if (Spec.IsValid())
            {
                TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

                if (ALastFPSCharacterBase* Shooter = Cast<ALastFPSCharacterBase>(GetInstigator()))
                    Shooter->Client_NotifyHitMarker();
            }
        }
    }

    Destroy();
}
