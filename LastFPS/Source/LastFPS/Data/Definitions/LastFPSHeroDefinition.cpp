#include "Data/Definitions/LastFPSHeroDefinition.h"

void ULastFPSHeroDefinition::GiveToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	// 공통 스탯 적용
	Super::GiveToAbilitySystem(ASC);

	// TODO: 히어로 전용 스타트업 처리(예: 로드아웃/패시브 이펙트)를 여기에 추가.
}
