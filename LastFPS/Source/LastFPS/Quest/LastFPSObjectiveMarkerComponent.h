#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSObjectiveMarkerComponent.generated.h"

class ULastFPSQuestSubsystem;

/**
 * 퀘스트 위치 목표(ReachLocation)의 월드 위치 소스.
 * 레벨에 배치한 액터(또는 NPC)에 붙여 LocationTag 로 퀘스트 서브시스템에 자신을 등록한다.
 * 도달 판정(거리)과 HUD 마커(Phase 3)가 이 태그로 위치를 조회한다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSObjectiveMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULastFPSObjectiveMarkerComponent();

	/** 이 지점을 식별하는 위치 태그 (목표의 TargetTag 와 매칭) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest")
	FGameplayTag LocationTag;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	bool bRegistered = false;
};
