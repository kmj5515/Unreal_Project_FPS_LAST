#include "AbilitySystem/Abilities/GA_EnemyMelee.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "Character/LastFPSCharacterBase.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Enemies/LastFPSEnemyMeleeAttackData.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Utility/LastFPSCombatAffiliation.h"
#include "Utility/LastFPSTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEnemyMelee, Log, All);

UGA_EnemyMelee::UGA_EnemyMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Enemy_Melee);
	SetAssetTags(Tags);
}

bool UGA_EnemyMelee::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ALastFPSCharacterBase* SourceCharacter =
		ActorInfo ? Cast<ALastFPSCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return SourceCharacter
		&& SourceCharacter->IsAlive()
		&& AttackData
		&& AttackData->AttackMontage
		&& AttackData->HitEventTag.IsValid()
		&& !AttackData->EffectsOnHit.IsEmpty();
}

void UGA_EnemyMelee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSCharacterBase* SourceCharacter = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo());
	if (!SourceCharacter || !SourceCharacter->IsAlive() || !AttackData || !AttackData->AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartHitEventTask();
	StartTraceEventTask();
	if (!HitEventTask || !TraceEventTask || !StartAttackMontage())
	{
		UE_LOG(LogLastFPSEnemyMelee, Error,
			TEXT("근접 공격 시작 실패: Source=%s, Data=%s, Montage=%s, EventTag=%s"),
			*GetNameSafe(SourceCharacter),
			*GetNameSafe(AttackData),
			*GetNameSafe(AttackData ? AttackData->AttackMontage : nullptr),
			AttackData ? *AttackData->HitEventTag.ToString() : TEXT("Invalid"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_EnemyMelee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	EndContinuousTrace();

	if (HitEventTask)
	{
		HitEventTask->EndTask();
		HitEventTask = nullptr;
	}

	if (TraceEventTask)
	{
		TraceEventTask->EndTask();
		TraceEventTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_EnemyMelee::StartAttackMontage()
{
	if (!AttackData || !AttackData->AttackMontage)
	{
		return false;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackData->AttackMontage,
		FMath::Max(AttackData->MontagePlayRate, 0.01f));
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyMelee::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyMelee::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyMelee::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
	return true;
}

void UGA_EnemyMelee::StartHitEventTask()
{
	if (!AttackData || !AttackData->HitEventTag.IsValid())
	{
		return;
	}

	HitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		AttackData->HitEventTag,
		nullptr,
		false,
		true);
	if (HitEventTask)
	{
		HitEventTask->EventReceived.AddDynamic(this, &UGA_EnemyMelee::OnHitEventReceived);
		HitEventTask->ReadyForActivation();
	}
}

void UGA_EnemyMelee::StartTraceEventTask()
{
	TraceEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		LastFPSGameplayTags::Event_Montage_MeleeTrace,
		nullptr,
		false,
		false);
	if (TraceEventTask)
	{
		TraceEventTask->EventReceived.AddDynamic(this, &UGA_EnemyMelee::OnTraceEventReceived);
		TraceEventTask->ReadyForActivation();
	}
}

void UGA_EnemyMelee::PerformMeleeHit(ALastFPSCharacterBase& SourceCharacter)
{
	UWorld* World = SourceCharacter.GetWorld();
	if (!World || !SourceCharacter.HasAuthority() || !AttackData)
	{
		return;
	}

	const FVector TraceStart = SourceCharacter.GetActorTransform().TransformPosition(AttackData->TraceStartOffset);
	const FVector TraceEnd = TraceStart
		+ SourceCharacter.GetActorForwardVector() * FMath::Max(AttackData->TraceDistance, 0.f);
	const float TraceRadius = FMath::Max(AttackData->TraceRadius, 1.f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyMeleeAttack), false, &SourceCharacter);
	QueryParams.AddIgnoredActor(&SourceCharacter);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	if (ShouldDrawDebug())
	{
		DrawDebugLine(GetCurrentActorInfo(), TraceStart, TraceEnd);
		DrawDebugSphere(GetCurrentActorInfo(), TraceStart, TraceRadius);
		DrawDebugSphere(GetCurrentActorInfo(), TraceEnd, TraceRadius);
	}

	TSet<AActor*> ProcessedActors;
	const AAIController* AIController = Cast<AAIController>(SourceCharacter.GetController());
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || ProcessedActors.Contains(TargetActor))
		{
			continue;
		}

		ProcessedActors.Add(TargetActor);
		const ALastFPSCharacterBase* TargetCharacter = Cast<ALastFPSCharacterBase>(TargetActor);
		if ((TargetCharacter && !TargetCharacter->IsAlive())
			|| LastFPSCombatAffiliation::AreFriendlyActors(&SourceCharacter, TargetActor)
			|| (AIController && !AIController->LineOfSightTo(TargetActor)))
		{
			continue;
		}

		if (ApplyEffectsToTarget(SourceCharacter, *TargetActor) && !AttackData->bHitMultipleTargets)
		{
			break;
		}
	}
}

void UGA_EnemyMelee::BeginContinuousTrace(ALastFPSCharacterBase& SourceCharacter)
{
	EndContinuousTrace();

	FVector InitialTraceCenter;
	if (!ResolveContinuousTraceCenter(SourceCharacter, InitialTraceCenter))
	{
		UE_LOG(LogLastFPSEnemyMelee, Error,
			TEXT("근접 연속 판정 시작 실패: Source=%s, Data=%s, Socket=%s"),
			*GetNameSafe(&SourceCharacter),
			*GetNameSafe(AttackData),
			AttackData ? *AttackData->ContinuousTraceSocketName.ToString() : TEXT("None"));
		return;
	}

	ContinuousTraceHitActors.Reset();
	PreviousContinuousTraceCenter = InitialTraceCenter;
	bContinuousTraceActive = true;
	SweepContinuousTrace(SourceCharacter, InitialTraceCenter);
}

void UGA_EnemyMelee::UpdateContinuousTrace(ALastFPSCharacterBase& SourceCharacter)
{
	if (!bContinuousTraceActive)
	{
		return;
	}

	FVector CurrentTraceCenter;
	if (!ResolveContinuousTraceCenter(SourceCharacter, CurrentTraceCenter))
	{
		EndContinuousTrace();
		return;
	}

	SweepContinuousTrace(SourceCharacter, CurrentTraceCenter);
}

void UGA_EnemyMelee::EndContinuousTrace()
{
	bContinuousTraceActive = false;
	PreviousContinuousTraceCenter = FVector::ZeroVector;
	ContinuousTraceHitActors.Reset();
}

bool UGA_EnemyMelee::ResolveContinuousTraceCenter(
	const ALastFPSCharacterBase& SourceCharacter,
	FVector& OutTraceCenter) const
{
	if (!AttackData || AttackData->ContinuousTraceSocketName.IsNone())
	{
		return false;
	}

	const USkeletalMeshComponent* Mesh = SourceCharacter.GetMesh();
	if (!Mesh || !Mesh->DoesSocketExist(AttackData->ContinuousTraceSocketName))
	{
		return false;
	}

	const FVector SocketLocation = Mesh->GetSocketLocation(AttackData->ContinuousTraceSocketName);
	float GroundZ = SourceCharacter.GetActorLocation().Z;
	if (const UCapsuleComponent* Capsule = SourceCharacter.GetCapsuleComponent())
	{
		GroundZ -= Capsule->GetScaledCapsuleHalfHeight();
	}

	OutTraceCenter = FVector(
		SocketLocation.X,
		SocketLocation.Y,
		GroundZ + FMath::Max(AttackData->ContinuousTraceCenterHeight, 0.f));
	return true;
}

void UGA_EnemyMelee::SweepContinuousTrace(
	ALastFPSCharacterBase& SourceCharacter,
	const FVector& CurrentTraceCenter)
{
	UWorld* World = SourceCharacter.GetWorld();
	if (!World || !SourceCharacter.HasAuthority() || !AttackData || !bContinuousTraceActive)
	{
		return;
	}

	const float TraceRadius = FMath::Max(AttackData->ContinuousTraceRadius, 1.f);
	const float TraceHalfHeight = FMath::Max(AttackData->ContinuousTraceHalfHeight, TraceRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyContinuousMeleeAttack), false, &SourceCharacter);
	QueryParams.AddIgnoredActor(&SourceCharacter);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		PreviousContinuousTraceCenter,
		CurrentTraceCenter,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeCapsule(TraceRadius, TraceHalfHeight),
		QueryParams);

#if ENABLE_DRAW_DEBUG
	if (ShouldDrawDebug())
	{
		::DrawDebugCapsule(
			World,
			PreviousContinuousTraceCenter,
			TraceHalfHeight,
			TraceRadius,
			FQuat::Identity,
			GetDebugColor(),
			false,
			GetDebugDrawTime(),
			0,
			GetDebugLineThickness());
		::DrawDebugCapsule(
			World,
			CurrentTraceCenter,
			TraceHalfHeight,
			TraceRadius,
			FQuat::Identity,
			GetDebugColor(),
			false,
			GetDebugDrawTime(),
			0,
			GetDebugLineThickness());
		DrawDebugLine(GetCurrentActorInfo(), PreviousContinuousTraceCenter, CurrentTraceCenter);
	}
#endif

	PreviousContinuousTraceCenter = CurrentTraceCenter;
	const AAIController* AIController = Cast<AAIController>(SourceCharacter.GetController());
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || ContinuousTraceHitActors.Contains(TargetActor))
		{
			continue;
		}

		const ALastFPSCharacterBase* TargetCharacter = Cast<ALastFPSCharacterBase>(TargetActor);
		if ((TargetCharacter && !TargetCharacter->IsAlive())
			|| LastFPSCombatAffiliation::AreFriendlyActors(&SourceCharacter, TargetActor)
			|| (AIController && !AIController->LineOfSightTo(TargetActor)))
		{
			continue;
		}

		ContinuousTraceHitActors.Add(TargetActor);
		if (ApplyEffectsToTarget(SourceCharacter, *TargetActor) && !AttackData->bHitMultipleTargets)
		{
			bContinuousTraceActive = false;
			break;
		}
	}
}

bool UGA_EnemyMelee::ApplyEffectsToTarget(
	ALastFPSCharacterBase& SourceCharacter,
	AActor& TargetActor) const
{
	UAbilitySystemComponent* SourceASC = SourceCharacter.GetAbilitySystemComponent();
	const IAbilitySystemInterface* TargetAbilitySystem = Cast<IAbilitySystemInterface>(&TargetActor);
	UAbilitySystemComponent* TargetASC =
		TargetAbilitySystem ? TargetAbilitySystem->GetAbilitySystemComponent() : nullptr;
	if (!SourceASC || !TargetASC || !AttackData)
	{
		return false;
	}

	bool bAppliedAnyEffect = false;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : AttackData->EffectsOnHit)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(AttackData);
		Context.AddInstigator(&SourceCharacter, &SourceCharacter);

		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (!Spec.IsValid())
		{
			continue;
		}

		if (LastFPSDamage::IsDamageGameplayEffect(EffectClass))
		{
			LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), AttackData->DamageRange);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		bAppliedAnyEffect = true;
	}

	return bAppliedAnyEffect;
}

void UGA_EnemyMelee::FinishCurrentAbility(const bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

void UGA_EnemyMelee::OnHitEventReceived(FGameplayEventData Payload)
{
	if (!AttackData || !Payload.EventTag.MatchesTagExact(AttackData->HitEventTag))
	{
		return;
	}

	// 몽타주에 배치된 각 Gameplay Event Notify가 독립적인 한 번의 타격 판정을 요청한다.
	if (ALastFPSCharacterBase* SourceCharacter = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		PerformMeleeHit(*SourceCharacter);
	}
}

void UGA_EnemyMelee::OnTraceEventReceived(FGameplayEventData Payload)
{
	ALastFPSCharacterBase* SourceCharacter = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo());
	if (!SourceCharacter || !SourceCharacter->HasAuthority())
	{
		return;
	}

	if (Payload.EventTag.MatchesTagExact(LastFPSGameplayTags::Event_Montage_MeleeTrace_Begin))
	{
		BeginContinuousTrace(*SourceCharacter);
	}
	else if (Payload.EventTag.MatchesTagExact(LastFPSGameplayTags::Event_Montage_MeleeTrace_Tick))
	{
		UpdateContinuousTrace(*SourceCharacter);
	}
	else if (Payload.EventTag.MatchesTagExact(LastFPSGameplayTags::Event_Montage_MeleeTrace_End))
	{
		UpdateContinuousTrace(*SourceCharacter);
		EndContinuousTrace();
	}
}

void UGA_EnemyMelee::OnMontageCompleted()
{
	FinishCurrentAbility(false);
}

void UGA_EnemyMelee::OnMontageCancelled()
{
	FinishCurrentAbility(true);
}

void UGA_EnemyMelee::OnMontageInterrupted()
{
	FinishCurrentAbility(true);
}
