#include "Character/Components/LastFPSGrapplingTargetingComponent.h"

#include "Character/LastFPSHero.h"
#include "Components/PrimitiveComponent.h"
#include "Data/Abilities/LastFPSGrapplingHookData.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

ULastFPSGrapplingTargetingComponent::ULastFPSGrapplingTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULastFPSGrapplingTargetingComponent::ConfigureTargeting(
	ULastFPSGrapplingHookData* InTargetingData)
{
	TargetingData = InTargetingData;
	RestartPreviewTimer();
}

bool ULastFPSGrapplingTargetingComponent::ResolveGrappleTarget(
	const ULastFPSGrapplingHookData& InTargetingData,
	FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const ALastFPSHero* Hero = ResolveHero();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Hero->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	if (const AController* Controller = Hero->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation
		+ ViewRotation.Vector() * InTargetingData.MaximumDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GrapplingHookTrace), false, Hero);
	QueryParams.AddIgnoredActor(Hero);
	if (!World->LineTraceSingleByChannel(
		OutHit,
		ViewLocation,
		TraceEnd,
		InTargetingData.TraceChannel,
		QueryParams))
	{
		return false;
	}

	const float TargetDistance = FVector::Distance(
		Hero->GetActorLocation(),
		OutHit.ImpactPoint);
	if (TargetDistance < InTargetingData.MinimumDistance
		|| TargetDistance > InTargetingData.MaximumDistance)
	{
		return false;
	}

	const UPrimitiveComponent* HitComponent = OutHit.GetComponent();
	return !InTargetingData.bRequireStaticSurface
		|| (HitComponent && HitComponent->GetMobility() == EComponentMobility::Static);
}

void ULastFPSGrapplingTargetingComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
	}

	TargetingData = nullptr;
	SetTargetAvailability(false);
	Super::EndPlay(EndPlayReason);
}

void ULastFPSGrapplingTargetingComponent::RestartPreviewTimer()
{
	UWorld* World = GetWorld();
	const ALastFPSHero* Hero = ResolveHero();
	if (!World)
	{
		SetTargetAvailability(false);
		return;
	}

	World->GetTimerManager().ClearTimer(PreviewTimerHandle);
	if (!TargetingData || !Hero || !Hero->IsLocallyControlled())
	{
		SetTargetAvailability(false);
		return;
	}

	RefreshTargetAvailability();
	World->GetTimerManager().SetTimer(
		PreviewTimerHandle,
		this,
		&ULastFPSGrapplingTargetingComponent::RefreshTargetAvailability,
		FMath::Max(TargetingData->TargetPreviewRefreshInterval, 0.02f),
		true);
}

void ULastFPSGrapplingTargetingComponent::RefreshTargetAvailability()
{
	const ALastFPSHero* Hero = ResolveHero();
	if (!TargetingData || !Hero || !Hero->IsLocallyControlled()
		|| !Hero->IsAlive()
		|| Hero->GetCombatState() != EMMCombatState::Idle)
	{
		SetTargetAvailability(false);
		return;
	}

	FHitResult TargetHit;
	SetTargetAvailability(ResolveGrappleTarget(*TargetingData, TargetHit));
}

void ULastFPSGrapplingTargetingComponent::SetTargetAvailability(
	const bool bNewTargetAvailable)
{
	if (bTargetAvailable == bNewTargetAvailable)
	{
		return;
	}

	bTargetAvailable = bNewTargetAvailable;
	OnTargetAvailabilityChanged.Broadcast(bTargetAvailable);
}

const ALastFPSHero* ULastFPSGrapplingTargetingComponent::ResolveHero() const
{
	return Cast<ALastFPSHero>(GetOwner());
}
