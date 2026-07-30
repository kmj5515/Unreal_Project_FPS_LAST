#include "Data/Definitions/LastFPSCaptureObjectiveDefinition.h"

#include "Components/ShapeComponent.h"
#include "Encounter/LastFPSTimedObjectiveComponent.h"
#include "GameFramework/Actor.h"

ULastFPSCaptureObjectiveDefinition::ULastFPSCaptureObjectiveDefinition()
{
	QuestObjectiveType = ELastFPSObjectiveType::CaptureZone;
	Duration = 8.f;
	// 점령은 진행이 멈췄다 재개되므로 남은 시간보다 차오르는 게이지가 읽기 쉽다.
	HudMode = ELastFPSObjectiveHudMode::Capture;
}

void ULastFPSCaptureObjectiveDefinition::ConfigureRuntimeObjective(
	AActor& Anchor,
	ULastFPSTimedObjectiveComponent& Objective) const
{
	// 배치물이 소유한 첫 번째 셰이프를 점령 볼륨으로 쓴다.
	// (볼륨을 여러 개 두는 구성은 아직 요구가 없어 지원하지 않는다.)
	UShapeComponent* Volume = Anchor.FindComponentByClass<UShapeComponent>();
	Objective.SetRequiredVolume(Volume);
	Objective.SetBlockedByEnemies(bBlockedByEnemies);
}
