#include "Data/Definitions/LastFPSEncounterObjectiveDefinition.h"

#include "Encounter/LastFPSTimedObjectiveComponent.h"
#include "GameFramework/Actor.h"

ULastFPSTimedObjectiveComponent* ULastFPSEncounterObjectiveDefinition::CreateRuntimeObjective(
	AActor& Anchor) const
{
	ULastFPSTimedObjectiveComponent* Objective = NewObject<ULastFPSTimedObjectiveComponent>(
		&Anchor,
		ULastFPSTimedObjectiveComponent::StaticClass(),
		MakeUniqueObjectName(&Anchor,ULastFPSTimedObjectiveComponent::StaticClass(),
			TEXT("TimedObjective")));
	if (!Objective)
	{
		return nullptr;
	}

	FLastFPSTimedObjectiveSettings Settings;
	Settings.Duration = FMath::Max(Duration, 0.01f);
	Settings.UpdateInterval = FMath::Max(UpdateInterval, 0.05f);
	Settings.QuestObjectiveType = QuestObjectiveType;
	Settings.ZoneTag = ZoneTag;
	Settings.DisplayLabel = DisplayLabel;
	Settings.HudMode = HudMode;
	Objective->ApplySettings(Settings);

	// 파생 클래스가 진행 조건과 실패 감시 대상을 연결한다.
	ConfigureRuntimeObjective(Anchor, *Objective);

	// 등록은 배선이 끝난 뒤에 한다 — 컴포넌트가 활성화되며 참조를 즉시 사용하기 때문이다.
	Anchor.AddInstanceComponent(Objective);
	Objective->RegisterComponent();
	return Objective;
}

bool ULastFPSEncounterObjectiveDefinition::IsConfigurationValid(FString& OutFailureReason) const
{
	if (ObjectiveTag.IsNone())
	{
		OutFailureReason = TEXT("ObjectiveTag 가 비어 있어 레벨 배치물과 짝지을 수 없습니다.");
		return false;
	}

	if (Duration <= 0.f)
	{
		OutFailureReason = TEXT("Duration 이 0 이하라 목표가 즉시 완료됩니다.");
		return false;
	}

	return true;
}
