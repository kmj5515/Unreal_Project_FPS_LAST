#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSStatusAnimationComponent.generated.h"

class UAbilitySystemComponent;
class USkeletalMeshComponent;

/** 상태 태그에 따른 애니메이션 반응을 캐릭터별로 적용한다. */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSStatusAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSStatusAnimationComponent();

	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);
	void PrepareForPhysicsSimulation();
	void ResumeStatusAnimationResponses();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindStatusTags();
	void UnbindStatusTags();
	void HandleStatusTagChanged(FGameplayTag StatusTag, int32 NewCount);
	void RefreshAnimationState();
	void ApplyAnimationPause();
	void RestoreAnimationPause();
	USkeletalMeshComponent* GetOwnerMesh() const;

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	TMap<FGameplayTag, FDelegateHandle> StatusTagDelegateHandles;
	bool bResponsesSuspended = false;
	bool bAnimationPauseApplied = false;
	bool bPreviousPauseAnims = false;
};
