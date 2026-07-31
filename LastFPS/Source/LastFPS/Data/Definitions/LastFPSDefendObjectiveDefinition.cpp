#include "Data/Definitions/LastFPSDefendObjectiveDefinition.h"

#include "Encounter/LastFPSTimedObjectiveComponent.h"
#include "GameFramework/Actor.h"
// TSubclassOf<UGameplayEffect> 의 유효성 검사에 완전 타입이 필요하다.
#include "GameplayEffect.h"

ULastFPSDefendObjectiveDefinition::ULastFPSDefendObjectiveDefinition()
{
	QuestObjectiveType = ELastFPSObjectiveType::DefendZone;
	// 방어는 "버티는" 감각이라 남은 시간 표기를 기본으로 한다.
	HudMode = ELastFPSObjectiveHudMode::Defend;
}

void ULastFPSDefendObjectiveDefinition::ConfigureRuntimeObjective(
	AActor& Anchor,
	ULastFPSTimedObjectiveComponent& Objective) const
{
	// 진행 조건 없음 = 시작하면 시간이 계속 흐른다. 배치물 자신이 지킬 대상이다.
	Objective.SetFailureWatchTarget(&Anchor);
	Objective.SetTargetInitEffect(DeviceInitEffect);
	Objective.SetTargetMaxHealth(DeviceMaxHealth);
}

bool ULastFPSDefendObjectiveDefinition::IsConfigurationValid(FString& OutFailureReason) const
{
	if (!Super::IsConfigurationValid(OutFailureReason))
	{
		return false;
	}

	// 둘 중 하나만 있으면 된다 — 효과를 지정했으면 그쪽이 우선한다.
	if (!DeviceInitEffect && DeviceMaxHealth <= 0.f)
	{
		OutFailureReason = TEXT("DeviceMaxHealth 와 DeviceInitEffect 가 모두 비어 지킬 대상의 체력을 초기화할 수 없습니다.");
		return false;
	}

	return true;
}
