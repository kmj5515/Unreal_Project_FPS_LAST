#include "AbilitySystem/Actors/LastFPSOrbitingProjectileEmitter.h"

#include "AbilitySystemInterface.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/Projectiles/LastFPSAbilityProjectileData.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectiles/LastFPSProjectile.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "Projectiles/LastFPSProjectileAimUtility.h"
#include "TimerManager.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Engine/OverlapResult.h"
#include "UObject/ConstructorHelpers.h"

FLastFPSOrbitingProjectileEmitterConfig::FLastFPSOrbitingProjectileEmitterConfig()
{
	SlotOffsets.Add(FVector::ZeroVector);
	SlotOffsets.Add(FVector(0.f, -35.f, -45.f));
	SlotOffsets.Add(FVector(0.f, 35.f, -45.f));
}

ALastFPSOrbitingProjectileEmitter::ALastFPSOrbitingProjectileEmitter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProbeMesh"));
	ProbeMesh->SetupAttachment(SceneRoot);
	ProbeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProbeMesh->SetGenerateOverlapEvents(false);
	ProbeMesh->SetRelativeScale3D(FVector(0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultProbeMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultProbeMesh.Succeeded())
	{
		ProbeMesh->SetStaticMesh(DefaultProbeMesh.Object);
	}
}

void ALastFPSOrbitingProjectileEmitter::BeginPlay()
{
	Super::BeginPlay();
}

void ALastFPSOrbitingProjectileEmitter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ALastFPSOrbitingProjectileEmitter::InitializeEmitter(
	ALastFPSCharacterBase* InSourceCharacter,
	const int32 InSlotIndex,
	const FLastFPSOrbitingProjectileEmitterConfig& InConfig,
	const float InBaseDamageOverride)
{
	SourceCharacter = InSourceCharacter;
	SlotIndex = FMath::Max(0, InSlotIndex);
	Config = InConfig;
	BaseDamageOverride = FMath::Max(InBaseDamageOverride, 0.f);

	SetOwner(InSourceCharacter);
	SetInstigator(InSourceCharacter);

	UpdateFollowLocation(0.f, true);
	RestartFireTimer();
}

AActor* ALastFPSOrbitingProjectileEmitter::GetSourceActor() const
{
	return SourceCharacter.Get();
}

int32 ALastFPSOrbitingProjectileEmitter::GetSlotIndex() const
{
	return SlotIndex;
}

void ALastFPSOrbitingProjectileEmitter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (!SourceCharacter || !SourceCharacter->IsAlive())
	{
		Destroy();
		return;
	}

	UpdateFollowLocation(DeltaSeconds, false);
}

void ALastFPSOrbitingProjectileEmitter::RestartFireTimer()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FireTimerHandle);
	if (!CanFireProjectile())
	{
		return;
	}

	if (Config.bFireImmediately)
	{
		FireProjectile();
		if (IsActorBeingDestroyed())
		{
			return;
		}
	}

	World->GetTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&ALastFPSOrbitingProjectileEmitter::FireProjectile,
		Config.FireInterval,
		true);
}

void ALastFPSOrbitingProjectileEmitter::UpdateFollowLocation(const float DeltaSeconds, const bool bSnap)
{
	if (!SourceCharacter)
	{
		return;
	}

	const FRotator YawRotation(0.f, SourceCharacter->GetActorRotation().Yaw, 0.f);
	const FVector DesiredOffset = Config.RelativeOffset + GetSlotOffset();
	const FVector DesiredLocation = SourceCharacter->GetActorLocation() + YawRotation.RotateVector(DesiredOffset);

	if (bSnap || Config.FollowInterpSpeed <= 0.f)
	{
		SetActorLocation(DesiredLocation);
	}
	else
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaSeconds, Config.FollowInterpSpeed));
	}

	SetActorRotation(YawRotation);
}

bool ALastFPSOrbitingProjectileEmitter::CanFireProjectile() const
{
	return SourceCharacter
		&& SourceCharacter->IsAlive()
		&& Config.FireInterval > 0.f
		&& Config.ProjectileData
		&& Config.ProjectileData->ProjectileClass;
}

FVector ALastFPSOrbitingProjectileEmitter::GetSlotOffset() const
{
	if (Config.SlotOffsets.IsValidIndex(SlotIndex))
	{
		return Config.SlotOffsets[SlotIndex];
	}

	return Config.SlotOffset * SlotIndex;
}

void ALastFPSOrbitingProjectileEmitter::FireProjectile()
{
	if (!HasAuthority() || !CanFireProjectile())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CameraAimDirection = LastFPSProjectileAim::GetAimDirection(SourceCharacter);
	FVector SpawnLocation = GetActorLocation();
	const FRotator InitialSpawnRotation = CameraAimDirection.Rotation();
	SpawnLocation += InitialSpawnRotation.RotateVector(Config.ProjectileData->SpawnLocationOffset);

	FVector AimTarget = FVector::ZeroVector;
	if (!ResolveFireTarget(World, SpawnLocation, CameraAimDirection, AimTarget))
	{
		return;
	}

	FVector AimDirection = (AimTarget - SpawnLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = CameraAimDirection;
	}

	const FRotator SpawnRotation = AimDirection.Rotation();

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	ALastFPSProjectile* Projectile = nullptr;
	if (ULastFPSActorPoolSubsystem* Pool =
		World->GetSubsystem<ULastFPSActorPoolSubsystem>())
	{
		Projectile = Cast<ALastFPSProjectile>(Pool->AcquireActorByClass(
			Config.ProjectileData->ProjectileClass,
			SpawnTransform,
			SourceCharacter,
			SourceCharacter));
	}
	if (!Projectile)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = SourceCharacter;
		SpawnParams.Instigator = SourceCharacter;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Projectile = World->SpawnActor<ALastFPSProjectile>(
			Config.ProjectileData->ProjectileClass,
			SpawnTransform,
			SpawnParams);
	}

	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeGameplayProjectile(
		SourceCharacter,
		Config.ProjectileData->ImpactRules,
		Config.ProjectileData->EffectsOnHit,
		Config.ProjectileData->VisualData,
		BaseDamageOverride,
		&Config.ProjectileData->CollisionSettings,
		&Config.ProjectileData->AudioSettings);

	if (Projectile->ProjectileMovement)
	{
		Projectile->ProjectileMovement->Velocity = AimDirection * Config.ProjectileData->ProjectileSpeed;
	}

	++FireCount;
	if (Config.MaxFireCount > 0 && FireCount >= Config.MaxFireCount)
	{
		Destroy();
	}
}

bool ALastFPSOrbitingProjectileEmitter::ResolveFireTarget(
	UWorld* World,
	const FVector& SpawnLocation,
	const FVector& CameraAimDirection,
	FVector& OutTargetLocation) const
{
	if (Config.bUseAutoTargeting)
	{
		if (const AActor* TargetActor = FindBestTargetActor(World, SpawnLocation, CameraAimDirection))
		{
			OutTargetLocation = ResolveTargetLocation(TargetActor);
			return true;
		}
	}

	if (Config.bRequireTargetToFire)
	{
		return false;
	}

	OutTargetLocation = LastFPSProjectileAim::GetAimTarget(
		World,
		SourceCharacter,
		CameraAimDirection,
		Config.ProjectileData->AimTraceRange);
	return true;
}

AActor* ALastFPSOrbitingProjectileEmitter::FindBestTargetActor(
	UWorld* World,
	const FVector& SpawnLocation,
	const FVector& CameraAimDirection) const
{
	if (!World || !SourceCharacter || Config.TargetSearchRange <= 0.f)
	{
		return nullptr;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProbeTargetSearch), false, SourceCharacter);
	QueryParams.AddIgnoredActor(SourceCharacter);
	QueryParams.AddIgnoredActor(this);

	TArray<AActor*> AttachedActors;
	SourceCharacter->GetAttachedActors(AttachedActors);
	QueryParams.AddIgnoredActors(AttachedActors);

	const FVector SearchLocation = SourceCharacter->GetActorLocation();
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		SearchLocation,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(Config.TargetSearchRange),
		QueryParams);
	if (!bHasOverlap)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	LastFPSProjectileAim::GetAimViewPoint(SourceCharacter, ViewLocation, ViewRotation);

	const FVector AimDirection = CameraAimDirection.IsNearlyZero()
		? ViewRotation.Vector().GetSafeNormal()
		: CameraAimDirection.GetSafeNormal();
	const float ConeCos = FMath::Cos(FMath::DegreesToRadians(Config.TargetAimConeAngle));

	AActor* BestConeTarget = nullptr;
	AActor* BestFallbackTarget = nullptr;
	float BestConeScore = TNumericLimits<float>::Max();
	float BestFallbackDistanceSq = TNumericLimits<float>::Max();

	TSet<AActor*> VisitedTargets;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValidTargetActor(Candidate) || VisitedTargets.Contains(Candidate))
		{
			continue;
		}
		VisitedTargets.Add(Candidate);

		const FVector TargetLocation = ResolveTargetLocation(Candidate);
		const FVector ViewToTarget = TargetLocation - ViewLocation;
		const float ViewDistance = ViewToTarget.Size();
		if (ViewDistance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float SpawnDistanceSq = FVector::DistSquared(SpawnLocation, TargetLocation);
		if (SpawnDistanceSq < BestFallbackDistanceSq)
		{
			BestFallbackDistanceSq = SpawnDistanceSq;
			BestFallbackTarget = Candidate;
		}

		const FVector ViewDirectionToTarget = ViewToTarget / ViewDistance;
		if (FVector::DotProduct(AimDirection, ViewDirectionToTarget) < ConeCos)
		{
			continue;
		}

		const FVector ClosestPointOnAimLine = ViewLocation + AimDirection * FVector::DotProduct(ViewToTarget, AimDirection);
		const float AimLineDistanceSq = FVector::DistSquared(TargetLocation, ClosestPointOnAimLine);
		const float ConeScore = AimLineDistanceSq + SpawnDistanceSq * 0.01f;
		if (ConeScore < BestConeScore)
		{
			BestConeScore = ConeScore;
			BestConeTarget = Candidate;
		}
	}

	return BestConeTarget ? BestConeTarget : BestFallbackTarget;
}

bool ALastFPSOrbitingProjectileEmitter::IsValidTargetActor(const AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == SourceCharacter || TargetActor == this || TargetActor == GetOwner())
	{
		return false;
	}

	// 조준 단계에서 아군을 제외한다. 임팩트 단계에서 데미지는 막히지만 연출과 발사가 낭비된다.
	if (LastFPSCombatAffiliation::AreFriendlyActors(SourceCharacter.Get(), TargetActor))
	{
		return false;
	}

	if (const ALastFPSCharacterBase* TargetCharacter = Cast<ALastFPSCharacterBase>(TargetActor))
	{
		return TargetCharacter->IsAlive();
	}

	return Cast<IAbilitySystemInterface>(TargetActor) != nullptr;
}

FVector ALastFPSOrbitingProjectileEmitter::ResolveTargetLocation(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return FVector::ZeroVector;
	}

	FVector Origin;
	FVector Extent;
	TargetActor->GetActorBounds(true, Origin, Extent);
	return Origin;
}
