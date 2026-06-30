#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Utility/LastFPSDamageCalculation.h"

ALastFPSAreaEffectActor::ALastFPSAreaEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(SceneRoot);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	EffectNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectNiagaraComponent"));
	EffectNiagaraComponent->SetupAttachment(SceneRoot);
	EffectNiagaraComponent->bAutoActivate = false;
}

void ALastFPSAreaEffectActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureArea();

	if (!HasAuthority())
	{
		return;
	}

	ApplyAreaEffects();

	if (AreaConfig.DamageInterval > 0.f && (AreaConfig.DamageEffect || !AreaConfig.TargetEffects.IsEmpty()))
	{
		GetWorldTimerManager().SetTimer(
			DamageTimerHandle,
			this,
			&ALastFPSAreaEffectActor::ApplyAreaEffects,
			AreaConfig.DamageInterval,
			true);
	}

	if (AreaConfig.Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ALastFPSAreaEffectActor::FinishArea,
			AreaConfig.Duration,
			false);
	}
}

void ALastFPSAreaEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALastFPSAreaEffectActor, AreaConfig);
}

void ALastFPSAreaEffectActor::InitializeAreaEffect(
	AActor* InSourceActor,
	UAbilitySystemComponent* InSourceASC,
	const FLastFPSAreaEffectConfig& InAreaConfig)
{
	SourceActor = InSourceActor;
	SourceASC = InSourceASC;
	AreaConfig = InAreaConfig;
	ConfigureArea();
}

void ALastFPSAreaEffectActor::OnRep_AreaConfig()
{
	ConfigureArea();
}

void ALastFPSAreaEffectActor::ConfigureArea()
{
	if (AreaSphere)
	{
		AreaSphere->SetSphereRadius(AreaConfig.Radius, true);
	}

	if (EffectNiagaraComponent)
	{
		EffectNiagaraComponent->SetAsset(AreaConfig.EffectNiagaraSystem);
		if (AreaConfig.EffectNiagaraSystem)
		{
			EffectNiagaraComponent->Activate(true);
		}
		else
		{
			EffectNiagaraComponent->Deactivate();
		}
	}

	DrawAreaDebug();
}

void ALastFPSAreaEffectActor::ApplyAreaEffects()
{
	if (!HasAuthority())
	{
		return;
	}

	DrawAreaDebug();

	TArray<AActor*> TargetActors;
	CollectTargets(TargetActors);

	for (AActor* TargetActor : TargetActors)
	{
		if (!DoesTargetPassTags(TargetActor))
		{
			continue;
		}

		DrawTargetDebug(TargetActor);
		ApplyEffectToTarget(TargetActor, AreaConfig.DamageEffect, true);

		for (const TSubclassOf<UGameplayEffect>& TargetEffect : AreaConfig.TargetEffects)
		{
			ApplyEffectToTarget(TargetActor, TargetEffect, false);
		}
	}
}

void ALastFPSAreaEffectActor::ApplyEffectToTarget(
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> EffectClass,
	bool bApplyDamage)
{
	if (!TargetActor || !EffectClass || !SourceASC.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(SourceActor.Get(), this);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
	if (!Spec.IsValid())
	{
		return;
	}

	if (bApplyDamage || LastFPSDamage::IsDamageGameplayEffect(EffectClass))
	{
		LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), AreaConfig.DamageRange);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void ALastFPSAreaEffectActor::FinishArea()
{
	Destroy();
}

void ALastFPSAreaEffectActor::CollectTargets(TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	UWorld* World = GetWorld();
	if (!World || AreaConfig.Radius <= 0.f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(AreaConfig.Radius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AreaEffectOverlap), false, SourceActor.Get());
	if (SourceActor.IsValid())
	{
		QueryParams.AddIgnoredActor(SourceActor.Get());
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		Shape,
		QueryParams);
	if (!bHasOverlaps)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> UniqueActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		const TWeakObjectPtr<AActor> TargetKey(TargetActor);
		if (!TargetActor || UniqueActors.Contains(TargetKey))
		{
			continue;
		}

		UniqueActors.Add(TargetKey);
		OutTargets.Add(TargetActor);
	}
}

bool ALastFPSAreaEffectActor::DoesTargetPassTags(AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);

	if (!AreaConfig.RequiredTargetTags.IsEmpty() && !OwnedTags.HasAll(AreaConfig.RequiredTargetTags))
	{
		return false;
	}

	if (!AreaConfig.BlockedTargetTags.IsEmpty() && OwnedTags.HasAny(AreaConfig.BlockedTargetTags))
	{
		return false;
	}

	return true;
}

UAbilitySystemComponent* ALastFPSAreaEffectActor::GetAbilitySystemComponentFromActor(AActor* Actor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}

void ALastFPSAreaEffectActor::DrawAreaDebug() const
{
	UWorld* World = GetWorld();
	if (!World || !AreaConfig.bDrawDebug)
	{
		return;
	}

	DrawDebugSphere(
		World,
		GetActorLocation(),
		AreaConfig.Radius,
		32,
		AreaConfig.DebugColor.ToFColor(true),
		false,
		AreaConfig.DebugDrawTime,
		0,
		2.f);
}

void ALastFPSAreaEffectActor::DrawTargetDebug(AActor* TargetActor) const
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor || !AreaConfig.bDrawDebug)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	DrawDebugPoint(World, TargetLocation, 12.f, AreaConfig.DebugColor.ToFColor(true), false, AreaConfig.DebugDrawTime);
	DrawDebugLine(World, GetActorLocation(), TargetLocation, AreaConfig.DebugColor.ToFColor(true), false, AreaConfig.DebugDrawTime, 0, 2.f);
}
