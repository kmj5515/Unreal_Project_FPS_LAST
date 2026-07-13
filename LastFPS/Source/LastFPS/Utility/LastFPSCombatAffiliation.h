#pragma once

#include "CoreMinimal.h"

namespace LastFPSCombatAffiliation
{
	/** 현재 전투 규칙에서 서로에게 공격 효과를 적용하면 안 되는 같은 진영인지 확인한다. */
	LASTFPS_API bool AreFriendlyActors(const AActor* SourceActor, const AActor* TargetActor);
}
