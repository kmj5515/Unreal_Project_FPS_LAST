#include "AbilitySystem/Actors/LastFPSAreaEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "Utility/LastFPSCombatAffiliation.h"
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
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	EffectNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectNiagaraComponent"));
	EffectNiagaraComponent->SetupAttachment(SceneRoot);
	EffectNiagaraComponent->bAutoActivate = false;

	AttachedAudioComponent =
		CreateDefaultSubobject<UAudioComponent>(TEXT("AttachedAudioComponent"));
	AttachedAudioComponent->SetupAttachment(SceneRoot);
	AttachedAudioComponent->bAutoActivate = false;
	AttachedAudioComponent->bAllowSpatialization = true;
}

void ALastFPSAreaEffectActor::BeginPlay()
{
	Super::BeginPlay();

	StartAreaEffect();
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
	// 같은 설정으로 재시전해도 복제 dirty 판정이 서도록 시퀀스를 올린다. 상세는 헤더 주석 참고.
	AreaConfig.ActivationSequence = ++AreaActivationSequence;
	if (HasActorBegunPlay())
	{
		StartAreaEffect();
	}
	else
	{
		ConfigureArea();
	}
	ForceNetUpdate();
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
		if (!AreaConfig.RadiusNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(AreaConfig.RadiusNiagaraParameterName, AreaConfig.Radius);
		}

		if (!AreaConfig.VisualRadiusNiagaraParameterName.IsNone())
		{
			const float VisualRadius = AreaConfig.VisualRadius > 0.f ? AreaConfig.VisualRadius : AreaConfig.Radius * 2.f;
			EffectNiagaraComponent->SetVariableFloat(AreaConfig.VisualRadiusNiagaraParameterName, VisualRadius);
		}

		if (!AreaConfig.DurationNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableFloat(AreaConfig.DurationNiagaraParameterName, AreaConfig.Duration);
		}

		if (!AreaConfig.SurfaceNormalNiagaraParameterName.IsNone())
		{
			EffectNiagaraComponent->SetVariableVec3(AreaConfig.SurfaceNormalNiagaraParameterName, GetActorUpVector());
		}

		if (AreaConfig.EffectNiagaraSystem)
		{
			EffectNiagaraComponent->Activate(true);
		}
		else
		{
			EffectNiagaraComponent->Deactivate();
		}
	}

	ConfigureAttachedAudio();
	DrawAreaDebug();
}

void ALastFPSAreaEffectActor::ConfigureAttachedAudio()
{
	if (!AttachedAudioComponent)
	{
		return;
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		StopAttachedAudio();
		return;
	}

	USoundBase* ConfiguredSound = AreaConfig.AttachedSound.Get();
	if (!ConfiguredSound)
	{
		StopAttachedAudio();
		return;
	}

	const bool bSoundChanged =
		AttachedAudioComponent->Sound != ConfiguredSound;
	if (bSoundChanged)
	{
		AttachedAudioComponent->Stop();
		AttachedAudioComponent->SetSound(ConfiguredSound);
	}

	AttachedAudioComponent->SetVolumeMultiplier(
		FMath::Max(AreaConfig.SoundVolumeMultiplier, 0.f));
	AttachedAudioComponent->SetPitchMultiplier(
		FMath::Max(AreaConfig.SoundPitchMultiplier, 0.01f));
	if (bSoundChanged || !AttachedAudioComponent->IsPlaying())
	{
		AttachedAudioComponent->Play(
			FMath::Max(AreaConfig.SoundStartTime, 0.f));
	}
}

void ALastFPSAreaEffectActor::StopAttachedAudio()
{
	if (!AttachedAudioComponent)
	{
		return;
	}

	AttachedAudioComponent->Stop();
	AttachedAudioComponent->SetSound(nullptr);
}

void ALastFPSAreaEffectActor::StartAreaEffect()
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);
	ConfigureArea();

	if (!HasAuthority())
	{
		return;
	}

	ApplyAreaEffects();

	if (AreaConfig.DamageInterval > 0.f
		&& (AreaConfig.DamageEffect || !AreaConfig.TargetEffects.IsEmpty()))
	{
		GetWorldTimerManager().SetTimer(
			DamageTimerHandle,
			this,
			&ThisClass::ApplyAreaEffects,
			AreaConfig.DamageInterval,
			true);
	}

	if (AreaConfig.Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ThisClass::FinishArea,
			AreaConfig.Duration,
			false);
	}
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
		// 오버랩은 진영을 구분하지 않는다. 아군 제외는 여기서만 걸린다.
		if (AreaConfig.bIgnoreFriendlyTargets
			&& LastFPSCombatAffiliation::AreFriendlyActors(SourceActor.Get(), TargetActor))
		{
			continue;
		}

		if (!DoesTargetPassShape(TargetActor))
		{
			continue;
		}

		if (!DoesTargetPassTags(TargetActor))
		{
			continue;
		}

		DrawTargetDebug(TargetActor);
		const bool bDamageApplied = ApplyEffectToTarget(TargetActor, AreaConfig.DamageEffect, true);
		const bool bShouldApplyFollowupEffects = !AreaConfig.DamageEffect || bDamageApplied;
		if (!bShouldApplyFollowupEffects)
		{
			continue;
		}

		if (AreaConfig.DamageCooldownEffect)
		{
			ApplyEffectToTarget(TargetActor, AreaConfig.DamageCooldownEffect, false);
		}

		for (const TSubclassOf<UGameplayEffect>& TargetEffect : AreaConfig.TargetEffects)
		{
			if (TargetEffect == AreaConfig.DamageCooldownEffect)
			{
				continue;
			}

			ApplyEffectToTarget(TargetActor, TargetEffect, false);
		}
	}
}

bool ALastFPSAreaEffectActor::ApplyEffectToTarget(
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> EffectClass,
	bool bApplyDamage)
{
	if (!IsValid(TargetActor) || !EffectClass || !SourceASC.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!IsValid(TargetASC))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(SourceActor.Get(), this);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
	if (!Spec.IsValid() || !Spec.Data.IsValid() || !Spec.Data->Def)
	{
		return false;
	}

	if (bApplyDamage || LastFPSDamage::IsDamageGameplayEffect(EffectClass))
	{
		LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), AreaConfig.DamageRange);
	}

	const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return AppliedHandle.WasSuccessfullyApplied();
}

void ALastFPSAreaEffectActor::FinishArea()
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

void ALastFPSAreaEffectActor::OnAcquiredFromPool_Implementation()
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);
	SourceActor.Reset();
	SourceASC.Reset();
	AreaConfig = FLastFPSAreaEffectConfig();
	StopAttachedAudio();
	SetActorEnableCollision(true);
}

void ALastFPSAreaEffectActor::OnReleasedToPool_Implementation()
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);
	SourceActor.Reset();
	SourceASC.Reset();
	AreaConfig = FLastFPSAreaEffectConfig();
	StopAttachedAudio();

	if (AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (EffectNiagaraComponent)
	{
		EffectNiagaraComponent->DeactivateImmediate();
		EffectNiagaraComponent->SetAsset(nullptr);
	}
}

void ALastFPSAreaEffectActor::OnPrepareForPoolRenderWarmup_Implementation()
{
	ConfigureArea();
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

bool ALastFPSAreaEffectActor::DoesTargetPassShape(AActor* TargetActor) const
{
	if (!TargetActor || AreaConfig.Shape != ELastFPSAreaEffectShape::Cone)
	{
		return true;
	}

	const float ConeAngleDegrees = FMath::Clamp(AreaConfig.ConeAngleDegrees, 0.f, 360.f);
	if (ConeAngleDegrees >= 360.f)
	{
		return true;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}

	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.Normalize())
	{
		return true;
	}

	const float Dot = FVector::DotProduct(Forward, ToTarget.GetSafeNormal());
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(ConeAngleDegrees * 0.5f));
	return Dot >= MinDot;
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

	const FColor DebugColor = AreaConfig.DebugColor.ToFColor(true);
	if (AreaConfig.Shape == ELastFPSAreaEffectShape::Cone)
	{
		const FVector Origin = GetActorLocation();
		const float HalfAngleDegrees = FMath::Clamp(AreaConfig.ConeAngleDegrees, 0.f, 360.f) * 0.5f;
		FVector Forward = GetActorForwardVector();
		Forward.Z = 0.f;
		if (!Forward.Normalize())
		{
			Forward = FVector::ForwardVector;
		}

		const FVector LeftDirection = Forward.RotateAngleAxis(-HalfAngleDegrees, FVector::UpVector);
		const FVector RightDirection = Forward.RotateAngleAxis(HalfAngleDegrees, FVector::UpVector);
		DrawDebugLine(World, Origin, Origin + LeftDirection * AreaConfig.Radius, DebugColor, false, AreaConfig.DebugDrawTime, 0, 2.f);
		DrawDebugLine(World, Origin, Origin + RightDirection * AreaConfig.Radius, DebugColor, false, AreaConfig.DebugDrawTime, 0, 2.f);

		const int32 SegmentCount = FMath::Max(8, FMath::CeilToInt(AreaConfig.ConeAngleDegrees / 6.f));
		FVector PreviousPoint = Origin + LeftDirection * AreaConfig.Radius;
		for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float Angle = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
			const FVector Direction = Forward.RotateAngleAxis(Angle, FVector::UpVector);
			const FVector CurrentPoint = Origin + Direction * AreaConfig.Radius;
			DrawDebugLine(World, PreviousPoint, CurrentPoint, DebugColor, false, AreaConfig.DebugDrawTime, 0, 2.f);
			PreviousPoint = CurrentPoint;
		}
		return;
	}

	DrawDebugSphere(
		World,
		GetActorLocation(),
		AreaConfig.Radius,
		32,
		DebugColor,
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
