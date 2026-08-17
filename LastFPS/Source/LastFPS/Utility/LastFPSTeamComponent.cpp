#include "Utility/LastFPSTeamComponent.h"

ULastFPSTeamComponent::ULastFPSTeamComponent()
{
	// 진영은 값 하나를 들고 있을 뿐이라 매 프레임 갱신할 것이 없다.
	PrimaryComponentTick.bCanEverTick = false;
}

FGenericTeamId ULastFPSTeamComponent::GetGenericTeamId() const
{
	return FGenericTeamId(static_cast<uint8>(Team));
}

void ULastFPSTeamComponent::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	Team = static_cast<ELastFPSTeam>(NewTeamId.GetId());
}
