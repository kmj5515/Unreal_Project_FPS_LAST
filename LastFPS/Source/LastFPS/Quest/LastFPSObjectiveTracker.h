#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/Tables/LastFPSQuestData.h"

class ULastFPSEconomySubsystem;
class ULastFPSQuestSubsystem;

/** pull형 트래커가 진행 재계산에 참조하는 외부 상태 묶음. */
struct FLastFPSObjectiveEvalContext
{
	const ULastFPSEconomySubsystem* Economy = nullptr;
	const ULastFPSQuestSubsystem* Subsystem = nullptr;
	FVector PlayerLocation = FVector::ZeroVector;
	bool bHasPlayerLocation = false;
};

/** 외부 이벤트 1건 — push형 트래커가 목표와 매칭해 진행을 누적한다. */
struct FLastFPSObjectiveEvent
{
	ELastFPSObjectiveType Type = ELastFPSObjectiveType::AcquireItem;
	FGameplayTag Tag;	// KillTarget: 처치된 적 종류
	FName Id;			// TalkToNPC: 대화한 NPC 행 이름
	int32 Count = 1;
};

/**
 * 목표 유형별 판정 전략. 서브시스템이 (유형→트래커)로 보유하며, 진행 계산·이벤트 매칭의
 * 유형별 세부를 캡슐화한다 — 서브시스템에서 유형 switch 를 제거(개방-폐쇄, 신규 유형=트래커 추가).
 * - pull형(획득/위치): RecomputeProgress 로 절대 상태에서 재계산.
 * - push형(처치/대화): MatchesEvent 로 외부 이벤트를 누적.
 */
class ILastFPSObjectiveTracker
{
public:
	virtual ~ILastFPSObjectiveTracker() = default;

	/** 수락 시점 기준선(기본 0). AcquireItem 만 현재 보유량을 캡처. */
	virtual int32 CaptureBaseline(const FLastFPSQuestObjective& Objective, const FLastFPSObjectiveEvalContext& Context) const { return 0; }

	/** 절대 상태에서 진행 재계산. true=OutProgress 를 채움(pull형), false=이벤트 누적형이라 저장값 유지. */
	virtual bool RecomputeProgress(const FLastFPSQuestObjective& Objective, int32 Baseline, const FLastFPSObjectiveEvalContext& Context, int32& OutProgress) const { return false; }

	/** 외부 이벤트가 이 목표와 일치하는가(push형). 일치 시 서브시스템이 진행을 누적. */
	virtual bool MatchesEvent(const FLastFPSObjectiveEvent& Event, const FLastFPSQuestObjective& Objective) const { return false; }
};
