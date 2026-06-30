#include "AbilitySystem/Abilities/GA_ViolaIceAura.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Effects/GE_Skill2Cooldown.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/LastFPSHero.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Utility/LastFPSTags.h"

UGA_ViolaIceAura::UGA_ViolaIceAura()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	CooldownGameplayEffectClass = ULastFPSGE_Skill2Cooldown::StaticClass();

	AuraEffectEventTag = LastFPSGameplayTags::Event_Montage_ViolaIceAuraEffect;
	DamageRange.DamageElement = ELastFPSDamageElement::Ice;

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_Skill2);
	Tags.AddTag(LastFPSGameplayTags::Input_Skill2);
	SetAssetTags(Tags);
}

bool UGA_ViolaIceAura::CanActivateAbility(
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

	return ActorInfo && ActorInfo->AvatarActor.IsValid();
}

void UGA_ViolaIceAura::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	bAuraEffectCommitted = false;
	
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		Hero->SetCombatState(EMMCombatState::Casting);
	}
	
	if (AuraEffectEventTag.IsValid())
	{
		AuraEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			AuraEffectEventTag,
			nullptr,
			true,
			true);
		if (AuraEffectEventTask)
		{
			AuraEffectEventTask->EventReceived.AddDynamic(this, &UGA_ViolaIceAura::OnAuraEffectEvent);
			AuraEffectEventTask->ReadyForActivation();
		}
	}

	if (!AuraMontage)
	{
		if (!CommitAndApplyAuraEffect())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		return;
	}
	
	AuraMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AuraMontage,
		MontagePlayRate);
	if (!AuraMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AuraMontageTask->OnCompleted.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageCompleted);
	AuraMontageTask->OnCancelled.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageCancelled);
	AuraMontageTask->OnInterrupted.AddDynamic(this, &UGA_ViolaIceAura::OnAuraMontageInterrupted);
	AuraMontageTask->ReadyForActivation();
}

void UGA_ViolaIceAura::ApplyAuraEffect()
{
	if (!AuraEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(AuraEffect);
	if (Spec.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			Spec);
	}
}

bool UGA_ViolaIceAura::CommitAndApplyAuraEffect()
{
	if (bAuraEffectCommitted)
	{
		return true;
	}

	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return false;
	}

	bAuraEffectCommitted = true;
	ApplyAuraEffect();
	StartAuraLoop();
	DrawAuraSphere();
	return true;
}

void UGA_ViolaIceAura::StartAuraLoop()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishAura();
		return;
	}

	ApplyAuraTargetEffects();

	if (DamageInterval > 0.f && (AuraDamageEffect || !AuraTargetEffects.IsEmpty()))
	{
		World->GetTimerManager().SetTimer(
			AuraTargetEffectTimerHandle,
			this,
			&UGA_ViolaIceAura::ApplyAuraTargetEffects,
			DamageInterval,
			true);
	}

	if (AuraDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(
			AuraDurationTimerHandle,
			this,
			&UGA_ViolaIceAura::FinishAura,
			AuraDuration,
			false);
	}
	else
	{
		FinishAura();
	}
}

void UGA_ViolaIceAura::ApplyAuraTargetEffects()
{
	DrawAuraSphere();

	TArray<AActor*> TargetActors;
	GetActorsInAuraSphere(TargetActors);

	for (AActor* TargetActor : TargetActors)
	{
		if (!DoesTargetPassAuraTags(TargetActor))
		{
			continue;
		}

		DrawAuraTargetDebug(TargetActor);
		ApplyAuraTargetEffect(TargetActor, AuraDamageEffect, true);

		for (const TSubclassOf<UGameplayEffect>& TargetEffect : AuraTargetEffects)
		{
			ApplyAuraTargetEffect(TargetActor, TargetEffect, false);
		}
	}
}

void UGA_ViolaIceAura::ApplyAuraTargetEffect(
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> EffectClass,
	bool bApplyDamage)
{
	if (!TargetActor || !EffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectContext);
	if (!Spec.IsValid())
	{
		return;
	}

	if (bApplyDamage || LastFPSDamage::IsDamageGameplayEffect(EffectClass))
	{
		LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), DamageRange);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UGA_ViolaIceAura::FinishAura()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

bool UGA_ViolaIceAura::DoesTargetPassAuraTags(AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);

	if (!RequiredTargetTags.IsEmpty() && !OwnedTags.HasAll(RequiredTargetTags))
	{
		return false;
	}

	if (!BlockedTargetTags.IsEmpty() && OwnedTags.HasAny(BlockedTargetTags))
	{
		return false;
	}

	return true;
}

UAbilitySystemComponent* UGA_ViolaIceAura::GetAbilitySystemComponentFromActor(AActor* Actor) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}

FVector UGA_ViolaIceAura::GetAuraOrigin() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->GetActorLocation() : FVector::ZeroVector;
}

void UGA_ViolaIceAura::DrawAuraSphere() const
{
	if (AuraRadius <= 0.f)
	{
		return;
	}

	DrawDebugSphere(GetCurrentActorInfo(), GetAuraOrigin(), AuraRadius);
}

void UGA_ViolaIceAura::DrawAuraTargetDebug(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	DrawDebugPoint(GetCurrentActorInfo(), TargetLocation);
	DrawDebugLine(GetCurrentActorInfo(), GetAuraOrigin(), TargetLocation);
}

void UGA_ViolaIceAura::GetActorsInAuraSphere(TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : GetWorld();
	if (!World || !AvatarActor || AuraRadius <= 0.f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(AuraRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ViolaIceAuraSphere), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		GetAuraOrigin(),
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
		OutActors.Add(TargetActor);
	}
}

void UGA_ViolaIceAura::OnAuraEffectEvent(FGameplayEventData)
{
	if (!CommitAndApplyAuraEffect())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
	}
}

void UGA_ViolaIceAura::OnAuraMontageCompleted()
{
	if (!bAuraEffectCommitted)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UGA_ViolaIceAura::OnAuraMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_ViolaIceAura::OnAuraMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_ViolaIceAura::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AuraEffectEventTask)
	{
		AuraEffectEventTask->EndTask();
		AuraEffectEventTask = nullptr;
	}

	if (AuraMontageTask)
	{
		AuraMontageTask->EndTask();
		AuraMontageTask = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AuraTargetEffectTimerHandle);
		World->GetTimerManager().ClearTimer(AuraDurationTimerHandle);
	}
	
	if (ALastFPSHero* Hero = Cast<ALastFPSHero>(GetAvatarActorFromActorInfo()))
	{
		if (Hero->GetCombatState() == EMMCombatState::Casting)
		{
			Hero->SetCombatState(EMMCombatState::Idle);
		}
	}
	
	bAuraEffectCommitted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
