#include "AbilitySystem/Actors/LastFPSExpandingMeshAttackActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "NiagaraCommon.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraCommon.h"
#include "Utility/LastFPSCombatAffiliation.h"

namespace
{
	constexpr float DebugRingSurfaceOffset = 5.f;
	constexpr float NiagaraDesiredAgeSeekDelta = 1.f / 60.f;
}

ALastFPSExpandingMeshAttackActor::ALastFPSExpandingMeshAttackActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EffectNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectNiagaraComponent"));
	EffectNiagaraComponent->SetupAttachment(SceneRoot);
	EffectNiagaraComponent->bAutoActivate = false;
	EffectNiagaraComponent->SetAllowScalability(false);
	EffectNiagaraComponent->AddTickPrerequisiteActor(this);
}

void ALastFPSExpandingMeshAttackActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureAttack();
	if (HasAuthority())
	{
		PreviousOuterRadius = FMath::Max(AttackConfig.StartOuterRadius, 0.01f);
		ProcessRingHits(PreviousOuterRadius, PreviousOuterRadius);
		SetLifeSpan(
			FMath::Max(AttackConfig.ExpansionDuration, 0.01f)
			+ FMath::Max(AttackConfig.LifeAfterExpansion, 0.f));
	}
}

void ALastFPSExpandingMeshAttackActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateExpansion();
}

void ALastFPSExpandingMeshAttackActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALastFPSExpandingMeshAttackActor, VisualState);
	DOREPLIFETIME(ALastFPSExpandingMeshAttackActor, ExpansionStartServerTime);
}

void ALastFPSExpandingMeshAttackActor::InitializeAttack(
	AActor* InSourceActor,
	UAbilitySystemComponent* InSourceASC,
	const FLastFPSExpandingMeshAttackConfig& InAttackConfig)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceActor = InSourceActor;
	SourceASC = InSourceASC;
	AttackConfig = InAttackConfig;
	VisualState.EffectNiagaraSystem = InAttackConfig.EffectNiagaraSystem;
	VisualState.StartRadiusOffsetNiagaraParameterName = InAttackConfig.StartRadiusOffsetNiagaraParameterName;
	VisualState.EndRadiusOffsetNiagaraParameterName = InAttackConfig.EndRadiusOffsetNiagaraParameterName;
	VisualState.ExpansionDurationNiagaraParameterName = InAttackConfig.ExpansionDurationNiagaraParameterName;
	VisualState.ExpansionAlphaNiagaraParameterName = InAttackConfig.ExpansionAlphaNiagaraParameterName;
	VisualState.RingThicknessNiagaraParameterName = InAttackConfig.RingThicknessNiagaraParameterName;
	VisualState.MeshBaseOuterRadius = InAttackConfig.MeshBaseOuterRadius;
	VisualState.StartOuterRadius = InAttackConfig.StartOuterRadius;
	VisualState.EndOuterRadius = InAttackConfig.EndOuterRadius;
	VisualState.ExpansionDuration = InAttackConfig.ExpansionDuration;
	VisualState.RingThickness = InAttackConfig.RingThickness;
	ExpansionStartServerTime = GetSynchronizedWorldTime();
	PreviousOuterRadius = FMath::Max(InAttackConfig.StartOuterRadius, 0.01f);
	AffectedActors.Reset();
	ConfigureAttack();
	ForceNetUpdate();
}

void ALastFPSExpandingMeshAttackActor::OnRep_AttackState()
{
	ConfigureAttack();
}

void ALastFPSExpandingMeshAttackActor::ConfigureAttack()
{
	if (EffectNiagaraComponent)
	{
		EffectNiagaraComponent->SetAgeUpdateMode(ENiagaraAgeUpdateMode::DesiredAge);
		EffectNiagaraComponent->SetSeekDelta(NiagaraDesiredAgeSeekDelta);
		EffectNiagaraComponent->SetLockDesiredAgeDeltaTimeToSeekDelta(false);
		EffectNiagaraComponent->SetCanRenderWhileSeeking(true);
		EffectNiagaraComponent->SetDesiredAge(0.f);
		EffectNiagaraComponent->SetAsset(VisualState.EffectNiagaraSystem);
		const float BaseOuterRadius = FMath::Max(VisualState.MeshBaseOuterRadius, 0.01f);
		if (!VisualState.StartRadiusOffsetNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.StartRadiusOffsetNiagaraParameterName,
				VisualState.StartOuterRadius - BaseOuterRadius);
		}
		if (!VisualState.EndRadiusOffsetNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.EndRadiusOffsetNiagaraParameterName,
				VisualState.EndOuterRadius - BaseOuterRadius);
		}
		if (!VisualState.ExpansionDurationNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.ExpansionDurationNiagaraParameterName,
				FMath::Max(VisualState.ExpansionDuration, 0.01f));
		}
		if (!VisualState.ExpansionAlphaNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.ExpansionAlphaNiagaraParameterName,
				0.f);
		}
		if (!VisualState.RingThicknessNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.RingThicknessNiagaraParameterName,
				FMath::Max(VisualState.RingThickness, 0.01f));
		}

		if (VisualState.EffectNiagaraSystem)
		{
			EffectNiagaraComponent->Activate(true);
		}
		else
		{
			EffectNiagaraComponent->Deactivate();
		}
	}
}

void ALastFPSExpandingMeshAttackActor::UpdateExpansion()
{
	if (!VisualState.EffectNiagaraSystem)
	{
		return;
	}

	const float Duration = FMath::Max(VisualState.ExpansionDuration, 0.01f);
	const float Elapsed = FMath::Max(GetSynchronizedWorldTime() - ExpansionStartServerTime, 0.f);
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	if (EffectNiagaraComponent)
	{
		EffectNiagaraComponent->SetDesiredAge(FMath::Min(Elapsed, Duration));
		if (!VisualState.ExpansionAlphaNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(
				VisualState.ExpansionAlphaNiagaraParameterName,
				Alpha);
		}
	}
	const float CurrentOuterRadius = FMath::Lerp(
		FMath::Max(VisualState.StartOuterRadius, 0.01f),
		FMath::Max(VisualState.EndOuterRadius, 0.01f),
		Alpha);
	DrawCollisionDebug(CurrentOuterRadius);
	if (HasAuthority() && HasActorBegunPlay())
	{
		ProcessRingHits(PreviousOuterRadius, CurrentOuterRadius);
		PreviousOuterRadius = CurrentOuterRadius;
	}

	if (Alpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

void ALastFPSExpandingMeshAttackActor::DrawCollisionDebug(const float CurrentOuterRadius) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !AttackConfig.bDrawDebug || CurrentOuterRadius <= 0.f)
	{
		return;
	}

	const float CurrentInnerRadius = FMath::Max(
		CurrentOuterRadius - FMath::Max(AttackConfig.RingThickness, 0.01f),
		0.f);
	const FVector Center = GetActorLocation() + GetActorUpVector() * DebugRingSurfaceOffset;
	const int32 Segments = FMath::Clamp(AttackConfig.DebugCircleSegments, 12, 256);
	const float Thickness = FMath::Max(AttackConfig.DebugLineThickness, 0.1f);

	DrawDebugCircle(
		World,
		Center,
		CurrentOuterRadius,
		Segments,
		AttackConfig.DebugOuterColor.ToFColor(true),
		false,
		0.f,
		0,
		Thickness,
		GetActorForwardVector(),
		GetActorRightVector(),
		false);

	if (CurrentInnerRadius > 0.f)
	{
		DrawDebugCircle(
			World,
			Center,
			CurrentInnerRadius,
			Segments,
			AttackConfig.DebugInnerColor.ToFColor(true),
			false,
			0.f,
			0,
			Thickness,
			GetActorForwardVector(),
			GetActorRightVector(),
			false);
	}
#endif
}

void ALastFPSExpandingMeshAttackActor::ProcessRingHits(
	const float PreviousRadius,
	const float CurrentRadius)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || CurrentRadius <= 0.f)
	{
		return;
	}

	const float RingThickness = FMath::Max(AttackConfig.RingThickness, 0.01f);
	const float SweptInnerRadius = FMath::Max(
		FMath::Min(PreviousRadius, CurrentRadius) - RingThickness,
		0.f);
	const float SweptOuterRadius = FMath::Max(PreviousRadius, CurrentRadius);
	const float HitHalfHeight = FMath::Max(AttackConfig.HitHalfHeight, 0.01f);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExpandingRingAttack), false, SourceActor.Get());
	QueryParams.AddIgnoredActor(SourceActor.Get());

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		GetActorQuat(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(FVector(SweptOuterRadius, SweptOuterRadius, HitHalfHeight)),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || TargetActor == SourceActor.Get()
			|| AffectedActors.Contains(TargetActor) || !DoesTargetPassConditions(TargetActor))
		{
			continue;
		}

		const FVector LocalTargetLocation = GetActorTransform().InverseTransformPosition(
			TargetActor->GetActorLocation());
		float TargetRadius = 0.f;
		float TargetHalfHeight = 0.f;
		if (const UPrimitiveComponent* TargetRoot = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
		{
			TargetRadius = FMath::Max(TargetRoot->Bounds.BoxExtent.X, TargetRoot->Bounds.BoxExtent.Y);
			TargetHalfHeight = TargetRoot->Bounds.BoxExtent.Z;
		}

		const float DistanceFromCenter = FVector2D(LocalTargetLocation.X, LocalTargetLocation.Y).Size();
		const bool bInsideSweptRing = DistanceFromCenter + TargetRadius >= SweptInnerRadius
			&& DistanceFromCenter - TargetRadius <= SweptOuterRadius;
		const bool bInsideHeight = FMath::Abs(LocalTargetLocation.Z)
			<= HitHalfHeight + TargetHalfHeight;
		if (!bInsideSweptRing || !bInsideHeight)
		{
			continue;
		}

		AffectedActors.Add(TargetActor);
		ApplyEffectsToTarget(TargetActor);
	}
}

float ALastFPSExpandingMeshAttackActor::GetSynchronizedWorldTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}
	return World->GetTimeSeconds();
}

bool ALastFPSExpandingMeshAttackActor::DoesTargetPassConditions(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (AttackConfig.bIgnoreFriendlyTargets
		&& LastFPSCombatAffiliation::AreFriendlyActors(SourceActor.Get(), TargetActor))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);
	return (AttackConfig.RequiredTargetTags.IsEmpty()
			|| OwnedTags.HasAll(AttackConfig.RequiredTargetTags))
		&& (AttackConfig.BlockedTargetTags.IsEmpty()
			|| !OwnedTags.HasAny(AttackConfig.BlockedTargetTags));
}

bool ALastFPSExpandingMeshAttackActor::ApplyEffectsToTarget(AActor* TargetActor)
{
	bool bAppliedAnyEffect = false;
	if (AttackConfig.DamageEffect)
	{
		bAppliedAnyEffect |= ApplyEffectToTarget(
			TargetActor,
			AttackConfig.DamageEffect,
			true);
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : AttackConfig.TargetEffects)
	{
		if (!EffectClass || EffectClass == AttackConfig.DamageEffect)
		{
			continue;
		}
		bAppliedAnyEffect |= ApplyEffectToTarget(
			TargetActor,
			EffectClass,
			LastFPSDamage::IsDamageGameplayEffect(EffectClass));
	}
	return bAppliedAnyEffect;
}

bool ALastFPSExpandingMeshAttackActor::ApplyEffectToTarget(
	AActor* TargetActor,
	const TSubclassOf<UGameplayEffect> EffectClass,
	const bool bApplyDamage)
{
	if (!IsValid(TargetActor) || !EffectClass || !SourceASC.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(SourceActor.Get(), this);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
	if (!Spec.IsValid() || !Spec.Data.IsValid())
	{
		return false;
	}

	if (bApplyDamage)
	{
		LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), AttackConfig.DamageRange);
	}

	const FActiveGameplayEffectHandle AppliedHandle =
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return AppliedHandle.WasSuccessfullyApplied();
}

UAbilitySystemComponent* ALastFPSExpandingMeshAttackActor::GetAbilitySystemComponentFromActor(
	AActor* Actor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}
