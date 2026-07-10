#include "Data/Definitions/LastFPSCharacterDefinition.h"

#include "Data/Characters/LastFPSCharacterStatData.h"

void ULastFPSCharacterDefinition::GiveToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}

	// 공통: 캐릭터 기본 스탯(AttributeSet 초기값)을 ASC 에 적용.
	if (StatData)
	{
		StatData->ApplyToAbilitySystem(ASC);
	}

	// 어빌리티/스타트업 이펙트 부여는 서버 권한이 필요해
	// ALastFPSCharacterBase::GiveDefaultAbilities / ApplyDefaultEffects 에서 처리한다.
}
