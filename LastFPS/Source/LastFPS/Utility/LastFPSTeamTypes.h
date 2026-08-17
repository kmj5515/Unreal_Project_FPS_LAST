#pragma once

#include "CoreMinimal.h"
#include "LastFPSTeamTypes.generated.h"

/**
 * 게임에서 사용하는 기본 진영 식별자다.
 *
 * FGenericTeamId::NoTeam(255) 은 "진영 미상"이며, 피해 판정은 이를 아군으로 보지 않는다
 * (LastFPSCombatAffiliation::AreFriendlyActors 참고). 따라서 아군 피해를 막아야 하는 대상은
 * 반드시 여기의 값을 가져야 하고, 값을 비워 두면 모든 공격에 노출된다.
 */
UENUM()
enum class ELastFPSTeam : uint8
{
	Player = 0,
	Enemy = 1
};
