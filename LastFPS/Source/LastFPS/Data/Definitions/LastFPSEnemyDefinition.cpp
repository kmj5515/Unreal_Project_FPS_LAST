#include "Data/Definitions/LastFPSEnemyDefinition.h"

void ULastFPSEnemyDefinition::GiveToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	// 공통 스탯 적용
	Super::GiveToAbilitySystem(ASC);

	// TODO: 적 전용 스타트업 처리(예: AIProfile 기반 난이도 스케일링/버프)를 여기에 추가.
}
