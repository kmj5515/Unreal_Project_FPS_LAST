#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Cooldown.generated.h"

/**
 * 모든 어빌리티가 공유하는 쿨다운 효과의 "형태"만 정의한다.
 *
 * 지속시간과 부여 태그는 스펙에 실려 온다 — 시간은 밸런스 행, 태그는 스킬 정의 행이 소유한다
 * (ULastFPSActiveGameplayAbility::ApplyCooldown 참조).
 * 슬롯별로 클래스를 나누면 그 두 값이 데이터와 코드에 이중으로 존재하게 되므로 하나만 둔다.
 *
 * 태그를 여기서 부여하지 않는 것이 중요하다. DynamicGrantedTags 는 클래스가 부여하는 태그를
 * 대체하지 않고 더하므로, 여기에 태그가 있으면 스킬마다 엉뚱한 쿨다운이 함께 걸린다.
 */
UCLASS()
class LASTFPS_API ULastFPSGE_Cooldown : public UGameplayEffect
{
    GENERATED_BODY()

public:
    ULastFPSGE_Cooldown();
};
