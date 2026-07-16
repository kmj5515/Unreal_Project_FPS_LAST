#include "Character/Components/LastFPSStatusOverlayComponent.h"

#include "AbilitySystem/Status/LastFPSStatusEffectDataSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Status/LastFPSStatusEffectUIData.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStatusOverlay, Log, All);

ULastFPSStatusOverlayComponent::ULastFPSStatusOverlayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULastFPSStatusOverlayComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULastFPSStatusOverlayComponent, ReplicatedOverlayState);
}

void ULastFPSStatusOverlayComponent::InitializeWithAbilitySystem(
	UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (BoundAbilitySystemComponent.Get() == InAbilitySystemComponent)
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			RefreshOverlay();
		}
		else
		{
			ApplyReplicatedOverlayState();
		}
		return;
	}

	UnbindStatusTags();
	BoundAbilitySystemComponent = InAbilitySystemComponent;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BindStatusTags();
		RefreshOverlay();
	}
	else
	{
		ApplyReplicatedOverlayState();
	}
}

void ULastFPSStatusOverlayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindStatusTags();
	CancelPendingMaterialLoad();
	ClearOverlay();
	BoundAbilitySystemComponent.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULastFPSStatusOverlayComponent::BindStatusTags()
{
	UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get();
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	if (!AbilitySystem || !DataSubsystem)
	{
		return;
	}

	TArray<const FLastFPSStatusEffectUIData*> OverlayEntries;
	DataSubsystem->GetStatusOverlayEntries(OverlayEntries);
	for (const FLastFPSStatusEffectUIData* StatusData : OverlayEntries)
	{
		if (!StatusData)
		{
			continue;
		}

		FGameplayTagContainer TagsToBind;
		TagsToBind.AddTag(StatusData->StatusTag);
		if (StatusData->Overlay.StackTag.IsValid())
		{
			TagsToBind.AddTag(StatusData->Overlay.StackTag);
		}

		for (const FGameplayTag TagToBind : TagsToBind)
		{
			if (StatusTagDelegateHandles.Contains(TagToBind))
			{
				continue;
			}

			FDelegateHandle DelegateHandle = AbilitySystem->RegisterGameplayTagEvent(
				TagToBind,
				EGameplayTagEventType::AnyCountChange).AddUObject(
					this,
					&ULastFPSStatusOverlayComponent::HandleStatusTagChanged);
			StatusTagDelegateHandles.Add(TagToBind, DelegateHandle);
		}
	}
}

void ULastFPSStatusOverlayComponent::UnbindStatusTags()
{
	if (UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Entry : StatusTagDelegateHandles)
		{
			AbilitySystem->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::AnyCountChange)
				.Remove(Entry.Value);
		}
	}
	StatusTagDelegateHandles.Reset();
}

void ULastFPSStatusOverlayComponent::HandleStatusTagChanged(FGameplayTag, int32)
{
	RefreshOverlay();
}

void ULastFPSStatusOverlayComponent::RefreshOverlay()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ApplyReplicatedOverlayState();
		return;
	}

	UAbilitySystemComponent* AbilitySystem = BoundAbilitySystemComponent.Get();
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	if (!AbilitySystem || !DataSubsystem || !GetOwnerMesh())
	{
		SetReplicatedOverlayState(FGameplayTag(), 0.f);
		ClearOverlay();
		return;
	}

	TArray<const FLastFPSStatusEffectUIData*> OverlayEntries;
	DataSubsystem->GetStatusOverlayEntries(OverlayEntries);
	const FLastFPSStatusEffectUIData* BestStatusData = nullptr;
	for (const FLastFPSStatusEffectUIData* StatusData : OverlayEntries)
	{
		if (!StatusData || !IsOverlayActive(AbilitySystem, *StatusData))
		{
			continue;
		}

		if (!BestStatusData
			|| StatusData->Overlay.Priority > BestStatusData->Overlay.Priority
			|| (StatusData->Overlay.Priority == BestStatusData->Overlay.Priority
				&& StatusData->StatusTag.GetTagName().Compare(BestStatusData->StatusTag.GetTagName()) < 0))
		{
			BestStatusData = StatusData;
		}
	}

	if (!BestStatusData)
	{
		SetReplicatedOverlayState(FGameplayTag(), 0.f);
		CancelPendingMaterialLoad();
		ClearOverlay();
		return;
	}

	const float MixValue = CalculateOverlayMix(AbilitySystem, *BestStatusData);
	SetReplicatedOverlayState(BestStatusData->StatusTag, MixValue);
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	ApplyStatusData(*BestStatusData, MixValue);
}

void ULastFPSStatusOverlayComponent::SetReplicatedOverlayState(
	const FGameplayTag StatusTag,
	const float MixValue)
{
	if (ReplicatedOverlayState.StatusTag == StatusTag
		&& FMath::IsNearlyEqual(ReplicatedOverlayState.MixValue, MixValue, KINDA_SMALL_NUMBER))
	{
		return;
	}

	ReplicatedOverlayState.StatusTag = StatusTag;
	ReplicatedOverlayState.MixValue = MixValue;
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void ULastFPSStatusOverlayComponent::OnRep_OverlayState()
{
	ApplyReplicatedOverlayState();
}

void ULastFPSStatusOverlayComponent::ApplyReplicatedOverlayState()
{
	if (!ReplicatedOverlayState.StatusTag.IsValid())
	{
		CancelPendingMaterialLoad();
		ClearOverlay();
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	const FLastFPSStatusEffectUIData* StatusData = DataSubsystem
		? DataSubsystem->FindStatusEffectUIData(ReplicatedOverlayState.StatusTag)
		: nullptr;
	if (!StatusData || !StatusData->Overlay.bEnabled)
	{
		CancelPendingMaterialLoad();
		ClearOverlay();
		return;
	}

	ApplyStatusData(*StatusData, ReplicatedOverlayState.MixValue);
}

void ULastFPSStatusOverlayComponent::ApplyStatusData(
	const FLastFPSStatusEffectUIData& StatusData,
	const float MixValue)
{
	UMaterialInterface* Material = StatusData.Overlay.Material.Get();
	if (!Material)
	{
		RequestOverlayMaterial(StatusData);
		ClearOverlay();
		return;
	}

	CancelPendingMaterialLoad();
	ApplyOverlay(
		Material,
		StatusData.Overlay.StackMixParameterName,
		MixValue,
		StatusData.Overlay.bInterpolateStackMix,
		StatusData.Overlay.StackMixInterpSpeed);
}

bool ULastFPSStatusOverlayComponent::IsOverlayActive(
	const UAbilitySystemComponent* AbilitySystem,
	const FLastFPSStatusEffectUIData& StatusData) const
{
	if (!AbilitySystem || !StatusData.Overlay.bEnabled || StatusData.Overlay.Material.IsNull())
	{
		return false;
	}

	return AbilitySystem->GetTagCount(StatusData.StatusTag) > 0
		|| GetOverlayStackCount(AbilitySystem, StatusData) > 0;
}

float ULastFPSStatusOverlayComponent::CalculateOverlayMix(
	const UAbilitySystemComponent* AbilitySystem,
	const FLastFPSStatusEffectUIData& StatusData) const
{
	if (!AbilitySystem)
	{
		return 0.f;
	}

	if (AbilitySystem->GetTagCount(StatusData.StatusTag) > 0)
	{
		return StatusData.Overlay.StackMixEndValue;
	}

	const int32 StackCount = GetOverlayStackCount(AbilitySystem, StatusData);
	const int32 FullMixStackCount = FMath::Max(StatusData.Overlay.StackCountForFullMix, 1);
	const float StackAlpha = FMath::Clamp(
		static_cast<float>(StackCount) / static_cast<float>(FullMixStackCount),
		0.f,
		1.f);
	return FMath::Lerp(
		StatusData.Overlay.StackMixStartValue,
		StatusData.Overlay.StackMixEndValue,
		StackAlpha);
}

int32 ULastFPSStatusOverlayComponent::GetOverlayStackCount(
	const UAbilitySystemComponent* AbilitySystem,
	const FLastFPSStatusEffectUIData& StatusData) const
{
	if (!AbilitySystem || !StatusData.Overlay.StackTag.IsValid())
	{
		return 0;
	}

	FGameplayTagContainer StackTags;
	StackTags.AddTag(StatusData.Overlay.StackTag);
	const FGameplayEffectQuery StackQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StackTags);
	return AbilitySystem->GetAggregatedStackCount(StackQuery);
}

void ULastFPSStatusOverlayComponent::RequestOverlayMaterial(
	const FLastFPSStatusEffectUIData& StatusData)
{
	const FSoftObjectPath MaterialPath = StatusData.Overlay.Material.ToSoftObjectPath();
	if (!MaterialPath.IsValid() || (MaterialLoadHandle.IsValid() && PendingMaterialPath == MaterialPath))
	{
		return;
	}

	CancelPendingMaterialLoad();
	PendingMaterialPath = MaterialPath;
	MaterialLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		MaterialPath,
		FStreamableDelegate::CreateUObject(this, &ULastFPSStatusOverlayComponent::HandleOverlayMaterialLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!MaterialLoadHandle.IsValid())
	{
		UE_LOG(LogLastFPSStatusOverlay, Error,
			TEXT("캐릭터 '%s'의 상태 Overlay 머티리얼 '%s' 비동기 로드를 시작하지 못했습니다."),
			*GetNameSafe(GetOwner()), *MaterialPath.ToString());
		PendingMaterialPath.Reset();
	}
}

void ULastFPSStatusOverlayComponent::HandleOverlayMaterialLoaded()
{
	const FSoftObjectPath LoadedMaterialPath = PendingMaterialPath;
	MaterialLoadHandle.Reset();
	PendingMaterialPath.Reset();
	if (!LoadedMaterialPath.ResolveObject())
	{
		UE_LOG(LogLastFPSStatusOverlay, Error,
			TEXT("캐릭터 '%s'의 상태 Overlay 머티리얼 '%s' 비동기 로드가 실패했습니다."),
			*GetNameSafe(GetOwner()), *LoadedMaterialPath.ToString());
		return;
	}
	ApplyReplicatedOverlayState();
}

void ULastFPSStatusOverlayComponent::CancelPendingMaterialLoad()
{
	if (MaterialLoadHandle.IsValid())
	{
		MaterialLoadHandle->CancelHandle();
		MaterialLoadHandle.Reset();
	}
	PendingMaterialPath.Reset();
}

void ULastFPSStatusOverlayComponent::ApplyOverlay(
	UMaterialInterface* Material,
	const FName MixParameterName,
	const float MixValue,
	const bool bInterpolateMix,
	const float MixInterpSpeed)
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	UWorld* World = GetWorld();
	if (!Mesh || !Material || !World)
	{
		ClearOverlay();
		return;
	}

	const bool bCreateMID = !ActiveOverlayMID || ActiveOverlaySourceMaterial.Get() != Material;
	if (bCreateMID)
	{
		ActiveOverlaySourceMaterial = Material;
		ActiveOverlayMID = UMaterialInstanceDynamic::Create(Material, this);
		ActiveMixValue = 0.f;
	}

	ActiveMixParameterName = MixParameterName;
	TargetMixValue = MixValue;
	ActiveMixInterpSpeed = FMath::Max(MixInterpSpeed, 0.f);

	if (ActiveOverlayMID && !MixParameterName.IsNone())
	{
		if (bInterpolateMix
			&& ActiveMixInterpSpeed > 0.f
			&& !FMath::IsNearlyEqual(ActiveMixValue, TargetMixValue, KINDA_SMALL_NUMBER))
		{
			ActiveOverlayMID->SetScalarParameterValue(MixParameterName, ActiveMixValue);
			World->GetTimerManager().SetTimer(
				MixInterpolationTimerHandle,
				this,
				&ULastFPSStatusOverlayComponent::UpdateMixInterpolation,
				0.016f,
				true);
		}
		else
		{
			World->GetTimerManager().ClearTimer(MixInterpolationTimerHandle);
			ActiveMixValue = TargetMixValue;
			ActiveOverlayMID->SetScalarParameterValue(MixParameterName, ActiveMixValue);
		}
	}
	else
	{
		World->GetTimerManager().ClearTimer(MixInterpolationTimerHandle);
	}

	Mesh->SetOverlayMaterial(ActiveOverlayMID ? ActiveOverlayMID.Get() : Material);
}

void ULastFPSStatusOverlayComponent::UpdateMixInterpolation()
{
	UWorld* World = GetWorld();
	if (!World || !ActiveOverlayMID || ActiveMixParameterName.IsNone())
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(MixInterpolationTimerHandle);
		}
		return;
	}

	ActiveMixValue = FMath::FInterpTo(
		ActiveMixValue,
		TargetMixValue,
		World->GetDeltaSeconds(),
		ActiveMixInterpSpeed);
	if (FMath::IsNearlyEqual(ActiveMixValue, TargetMixValue, 0.001f))
	{
		ActiveMixValue = TargetMixValue;
		World->GetTimerManager().ClearTimer(MixInterpolationTimerHandle);
	}
	ActiveOverlayMID->SetScalarParameterValue(ActiveMixParameterName, ActiveMixValue);
}

void ULastFPSStatusOverlayComponent::ClearOverlay()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MixInterpolationTimerHandle);
	}
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		Mesh->SetOverlayMaterial(nullptr);
	}

	ActiveOverlayMID = nullptr;
	ActiveOverlaySourceMaterial = nullptr;
	ActiveMixParameterName = NAME_None;
	ActiveMixValue = 0.f;
	TargetMixValue = 0.f;
	ActiveMixInterpSpeed = 0.f;
}

USkeletalMeshComponent* ULastFPSStatusOverlayComponent::GetOwnerMesh() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character ? Character->GetMesh() : nullptr;
}
