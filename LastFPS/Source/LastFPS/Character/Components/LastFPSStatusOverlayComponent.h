#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSStatusOverlayComponent.generated.h"

class UAbilitySystemComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;
struct FLastFPSStatusEffectUIData;
struct FStreamableHandle;

USTRUCT()
struct FLastFPSReplicatedStatusOverlayState
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag StatusTag;

	UPROPERTY()
	float MixValue = 0.f;
};

/** 상태 태그를 캐릭터 메시의 Overlay 연출로 변환하는 캐릭터별 런타임 컴포넌트다. */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSStatusOverlayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSStatusOverlayComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindStatusTags();
	void UnbindStatusTags();
	void HandleStatusTagChanged(FGameplayTag StatusTag, int32 NewCount);
	void RefreshOverlay();
	void SetReplicatedOverlayState(FGameplayTag StatusTag, float MixValue);

	UFUNCTION()
	void OnRep_OverlayState();

	void ApplyReplicatedOverlayState();
	void ApplyStatusData(const FLastFPSStatusEffectUIData& StatusData, float MixValue);

	bool IsOverlayActive(
		const UAbilitySystemComponent* AbilitySystem,
		const FLastFPSStatusEffectUIData& StatusData) const;
	float CalculateOverlayMix(
		const UAbilitySystemComponent* AbilitySystem,
		const FLastFPSStatusEffectUIData& StatusData) const;
	int32 GetOverlayStackCount(
		const UAbilitySystemComponent* AbilitySystem,
		const FLastFPSStatusEffectUIData& StatusData) const;

	void RequestOverlayMaterial(const FLastFPSStatusEffectUIData& StatusData);
	void HandleOverlayMaterialLoaded();
	void CancelPendingMaterialLoad();
	void ApplyOverlay(
		UMaterialInterface* Material,
		FName MixParameterName,
		float MixValue,
		bool bInterpolateMix,
		float MixInterpSpeed);
	void UpdateMixInterpolation();
	void ClearOverlay();

	USkeletalMeshComponent* GetOwnerMesh() const;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	TMap<FGameplayTag, FDelegateHandle> StatusTagDelegateHandles;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveOverlayMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ActiveOverlaySourceMaterial;

	UPROPERTY(ReplicatedUsing=OnRep_OverlayState)
	FLastFPSReplicatedStatusOverlayState ReplicatedOverlayState;

	TSharedPtr<FStreamableHandle> MaterialLoadHandle;
	FSoftObjectPath PendingMaterialPath;
	FTimerHandle MixInterpolationTimerHandle;
	FName ActiveMixParameterName = NAME_None;
	float ActiveMixValue = 0.f;
	float TargetMixValue = 0.f;
	float ActiveMixInterpSpeed = 0.f;
};
