#include "AbilitySystem/Abilities/GA_ProbePassive.h"

#include "AbilitySystemComponent.h"
#include "Character/LastFPSCharacterBase.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Utility/LastFPSTags.h"

UGA_ProbePassive::UGA_ProbePassive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ProbeEmitterClass = ALastFPSOrbitingProjectileEmitter::StaticClass();

	FGameplayTagContainer Tags;
	Tags.AddTag(LastFPSGameplayTags::Ability_ProbePassive);
	SetAssetTags(Tags);

	TriggerAbilityTags.AddTag(LastFPSGameplayTags::Ability_Trigger_SpawnProbe);
}

bool UGA_ProbePassive::CanActivateAbility(
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

	const ALastFPSCharacterBase* SourceCharacter = ActorInfo
		? Cast<ALastFPSCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;
	return SourceCharacter && SourceCharacter->IsAlive();
}

void UGA_ProbePassive::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALastFPSCharacterBase* SourceCharacter = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo());
	if (!ActorInfo || !SourceCharacter || !SourceCharacter->HasAuthority() || !SourceCharacter->IsAlive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartListeningForAbilityActivations(ActorInfo);
}

void UGA_ProbePassive::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopListeningForAbilityActivations();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ProbePassive::StartListeningForAbilityActivations(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (AbilityActivatedDelegateHandle.IsValid()
		|| !ActorInfo
		|| !ActorInfo->AbilitySystemComponent.IsValid()
		|| !IsPrimaryListenerInstance())
	{
		return;
	}

	AbilityActivatedDelegateHandle = ActorInfo->AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(
		this,
		&UGA_ProbePassive::OnOwnerAbilityActivated);
}

void UGA_ProbePassive::StopListeningForAbilityActivations()
{
	if (!AbilityActivatedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AbilityActivatedCallbacks.Remove(AbilityActivatedDelegateHandle);
	}

	AbilityActivatedDelegateHandle.Reset();
}

void UGA_ProbePassive::OnOwnerAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (!IsPrimaryListenerInstance()
		|| !ShouldSpawnProbeForAbility(ActivatedAbility)
		|| ShouldSkipDuplicateActivation(ActivatedAbility))
	{
		return;
	}

	if (ALastFPSCharacterBase* SourceCharacter = Cast<ALastFPSCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		MarkHandledActivation(ActivatedAbility);

		if (SpawnProbeEmitter(SourceCharacter))
		{
			LastSpawnFrame = GFrameCounter;
		}
	}
}

bool UGA_ProbePassive::ShouldSpawnProbeForAbility(const UGameplayAbility* ActivatedAbility) const
{
	if (!ActivatedAbility || ActivatedAbility == this)
	{
		return false;
	}

	const FGameplayTagContainer& ActivatedAbilityTags = ActivatedAbility->GetAssetTags();
	if (ActivatedAbilityTags.HasTagExact(LastFPSGameplayTags::Ability_ProbePassive))
	{
		return false;
	}

	return TriggerAbilityTags.IsEmpty() || ActivatedAbilityTags.HasAnyExact(TriggerAbilityTags);
}

bool UGA_ProbePassive::IsPrimaryListenerInstance() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return true;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || Spec.Ability->GetClass() != GetClass() || !Spec.IsActive())
		{
			continue;
		}

		return Spec.GetPrimaryInstance() == this;
	}

	return true;
}

bool UGA_ProbePassive::ShouldSkipDuplicateActivation(const UGameplayAbility* ActivatedAbility) const
{
	return LastSpawnFrame == GFrameCounter
		|| (LastHandledAbility.Get() == ActivatedAbility && LastHandledFrame == GFrameCounter);
}

void UGA_ProbePassive::MarkHandledActivation(UGameplayAbility* ActivatedAbility)
{
	LastHandledAbility = ActivatedAbility;
	LastHandledFrame = GFrameCounter;
}

bool UGA_ProbePassive::SpawnProbeEmitter(ALastFPSCharacterBase* SourceCharacter) const
{
	UWorld* World = GetWorld();
	if (!World || !SourceCharacter || !ProbeEmitterClass)
	{
		return false;
	}

	int32 ExistingProbeCount = 0;
	const int32 SlotIndex = FindAvailableSlotIndex(World, SourceCharacter, ExistingProbeCount);
	if (ExistingProbeCount >= MaxProbeCount || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALastFPSOrbitingProjectileEmitter* ProbeEmitter = World->SpawnActor<ALastFPSOrbitingProjectileEmitter>(
		ProbeEmitterClass,
		SourceCharacter->GetActorLocation(),
		SourceCharacter->GetActorRotation(),
		SpawnParams);

	if (!ProbeEmitter)
	{
		return false;
	}

	ProbeEmitter->InitializeEmitter(SourceCharacter, SlotIndex, ProbeEmitterConfig);
	return true;
}

int32 UGA_ProbePassive::FindAvailableSlotIndex(
	UWorld* World,
	const ALastFPSCharacterBase* SourceCharacter,
	int32& OutExistingProbeCount) const
{
	OutExistingProbeCount = 0;
	if (!World || !SourceCharacter || !ProbeEmitterClass || MaxProbeCount <= 0)
	{
		return INDEX_NONE;
	}

	TSet<int32> UsedSlots;
	for (TActorIterator<ALastFPSOrbitingProjectileEmitter> It(World, ProbeEmitterClass); It; ++It)
	{
		const ALastFPSOrbitingProjectileEmitter* ExistingProbe = *It;
		if (!ExistingProbe || ExistingProbe->GetSourceActor() != SourceCharacter)
		{
			continue;
		}

		++OutExistingProbeCount;
		UsedSlots.Add(ExistingProbe->GetSlotIndex());
	}

	if (OutExistingProbeCount >= MaxProbeCount)
	{
		return INDEX_NONE;
	}

	for (int32 SlotIndex = 0; SlotIndex < MaxProbeCount; ++SlotIndex)
	{
		if (!UsedSlots.Contains(SlotIndex))
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}
