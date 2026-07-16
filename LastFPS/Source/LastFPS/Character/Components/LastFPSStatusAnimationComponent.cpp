#include "Character/Components/LastFPSStatusAnimationComponent.h"

#include "AbilitySystem/Status/LastFPSStatusEffectDataSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Status/LastFPSStatusEffectUIData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

ULastFPSStatusAnimationComponent::ULastFPSStatusAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULastFPSStatusAnimationComponent::InitializeWithAbilitySystem(
	UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (BoundAbilitySystemComponent.Get() == InAbilitySystemComponent)
	{
		RefreshAnimationState();
		return;
	}

	UnbindStatusTags();
	RestoreAnimationPause();
	BoundAbilitySystemComponent = InAbilitySystemComponent;
	BindStatusTags();
	RefreshAnimationState();
}

void ULastFPSStatusAnimationComponent::PrepareForPhysicsSimulation()
{
	bResponsesSuspended = true;
	RestoreAnimationPause();
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		Mesh->bPauseAnims = false;
	}
}

void ULastFPSStatusAnimationComponent::ResumeStatusAnimationResponses()
{
	bResponsesSuspended = false;
	RefreshAnimationState();
}

void ULastFPSStatusAnimationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindStatusTags();
	RestoreAnimationPause();
	BoundAbilitySystemComponent.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULastFPSStatusAnimationComponent::BindStatusTags()
{
	UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get();
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	if (!AbilitySystem || !DataSubsystem)
	{
		return;
	}

	TArray<const FLastFPSStatusEffectUIData*> AnimationEntries;
	DataSubsystem->GetStatusAnimationEntries(AnimationEntries);
	for (const FLastFPSStatusEffectUIData* StatusData : AnimationEntries)
	{
		if (!StatusData || !StatusData->StatusTag.IsValid()
			|| StatusTagDelegateHandles.Contains(StatusData->StatusTag))
		{
			continue;
		}

		FDelegateHandle DelegateHandle = AbilitySystem->RegisterGameplayTagEvent(
			StatusData->StatusTag,
			EGameplayTagEventType::NewOrRemoved).AddUObject(
				this,
				&ULastFPSStatusAnimationComponent::HandleStatusTagChanged);
		StatusTagDelegateHandles.Add(StatusData->StatusTag, DelegateHandle);
	}
}

void ULastFPSStatusAnimationComponent::UnbindStatusTags()
{
	if (UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Entry : StatusTagDelegateHandles)
		{
			AbilitySystem->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::NewOrRemoved)
				.Remove(Entry.Value);
		}
	}
	StatusTagDelegateHandles.Reset();
}

void ULastFPSStatusAnimationComponent::HandleStatusTagChanged(FGameplayTag, int32)
{
	RefreshAnimationState();
}

void ULastFPSStatusAnimationComponent::RefreshAnimationState()
{
	if (bResponsesSuspended)
	{
		RestoreAnimationPause();
		return;
	}

	const UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get();
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	if (!AbilitySystem || !DataSubsystem || !GetOwnerMesh())
	{
		RestoreAnimationPause();
		return;
	}

	TArray<const FLastFPSStatusEffectUIData*> AnimationEntries;
	DataSubsystem->GetStatusAnimationEntries(AnimationEntries);
	for (const FLastFPSStatusEffectUIData* StatusData : AnimationEntries)
	{
		if (StatusData
			&& StatusData->AnimationPolicy == ELastFPSStatusAnimationPolicy::PauseCurrentPose
			&& AbilitySystem->GetTagCount(StatusData->StatusTag) > 0)
		{
			ApplyAnimationPause();
			return;
		}
	}

	RestoreAnimationPause();
}

void ULastFPSStatusAnimationComponent::ApplyAnimationPause()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh || bAnimationPauseApplied)
	{
		return;
	}

	bPreviousPauseAnims = Mesh->bPauseAnims;
	Mesh->bPauseAnims = true;
	bAnimationPauseApplied = true;
}

void ULastFPSStatusAnimationComponent::RestoreAnimationPause()
{
	if (!bAnimationPauseApplied)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		Mesh->bPauseAnims = bPreviousPauseAnims;
	}
	bAnimationPauseApplied = false;
	bPreviousPauseAnims = false;
}

USkeletalMeshComponent* ULastFPSStatusAnimationComponent::GetOwnerMesh() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character ? Character->GetMesh() : nullptr;
}
