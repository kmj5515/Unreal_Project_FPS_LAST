#include "AbilitySystem/Effects/GE_Cooldown.h"

ULastFPSGE_Cooldown::ULastFPSGE_Cooldown()
{
    // 지속시간 값은 스펙의 SetDuration 이 정한다. 여기 리터럴을 두면 데이터가 비었을 때
    // 조용히 그 값으로 대체돼 밸런스 수정이 삼켜진다.
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
}
