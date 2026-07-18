#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_LastFPSMeleeTrace.generated.h"

/** 몽타주의 공격 구간을 Gameplay Event로 전달하는 근접 연속 판정 노티파이 상태다. */
UCLASS(meta=(DisplayName="Last FPS Melee Trace"))
class LASTFPS_API UAnimNotifyState_LastFPSMeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	static void SendTraceEvent(USkeletalMeshComponent* MeshComp, FGameplayTag EventTag, float EventMagnitude);
};
