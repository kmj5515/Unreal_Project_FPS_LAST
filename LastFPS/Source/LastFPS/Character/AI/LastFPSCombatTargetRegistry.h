#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSCombatTargetRegistry.generated.h"

/**
 * 시야 퍼셉션으로는 잡히지 않지만 적대 타깃이 되어야 하는 액터의 등록소다.
 *
 * 퍼셉션은 Pawn 을 전제로 하므로 방어 장치처럼 Pawn 이 아닌 대상은 영원히 타깃이 되지 못한다.
 * 그렇다고 전투 AI 에 "방어 장치" 분기를 넣으면 범용 시스템이 특정 콘텐츠를 알게 되므로,
 * 타깃이 되고 싶은 쪽이 스스로 등록하고 AI 는 등록소에만 질의한다.
 *
 * 등록·해제 수명은 등록 주체가 소유한다(목표 컴포넌트의 시작/중단 시점).
 * 매 틱 전수 스캔을 피하기 위한 캐시이기도 하다.
 */
UCLASS()
class LASTFPS_API ULastFPSCombatTargetRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 적대 타깃으로 등록한다. 같은 액터를 다시 등록하면 우선순위만 갱신한다.
	 * Priority 가 클수록 먼저 선택된다(동률이면 더 가까운 쪽).
	 */
	void RegisterTarget(AActor& Target, int32 Priority = 0);

	void UnregisterTarget(AActor& Target);

	/**
	 * From 기준 MaxDistance 안에서 가장 적합한 등록 타깃을 고른다.
	 * MaxDistance 가 0 이하면 거리 제한을 두지 않는다. 없으면 nullptr.
	 */
	AActor* FindBestTarget(const FVector& From, float MaxDistance) const;

	/** 등록된 타깃이 하나라도 있는가. AI 서비스가 질의 비용을 아끼는 데 쓴다. */
	bool HasTargets() const { return !Entries.IsEmpty(); }

	virtual void Deinitialize() override;

private:
	/** 등록 항목 1건. 레벨 액터 수명이라 약참조로 들고 조회 시 정리한다. */
	struct FEntry
	{
		TWeakObjectPtr<AActor> Target;
		int32 Priority = 0;
	};

	/** 파괴된 대상을 걸러낸다. 조회 시점에 정리하므로 별도 해제 누락에도 안전하다. */
	void PruneInvalidEntries() const;

	mutable TArray<FEntry> Entries;
};
