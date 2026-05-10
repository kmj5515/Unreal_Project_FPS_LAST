#include "Weapons/LastFPSProjectile.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ALastFPSProjectile::ALastFPSProjectile()
{
    bReplicates = true;

    CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
    CollisionComp->InitBoxExtent(FVector(2.5f, 1.f, 1.f));
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // VFX 전용, 충돌 없음
    RootComponent = CollisionComp;

    TrailParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticle"));
    TrailParticle->SetupAttachment(CollisionComp);
    TrailParticle->bAutoActivate = true;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed             = 30000.f;  // 300 m/s — 트레일 가시성 확보
    ProjectileMovement->MaxSpeed                 = 30000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale   = 0.f;      // 데미지는 LineTrace로 처리하므로 중력 불필요

    InitialLifeSpan = 1.5f;  // 1.5초에 450m 이동 후 소멸
}

void ALastFPSProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (TrailEffect)
        TrailParticle->SetTemplate(TrailEffect);
}
