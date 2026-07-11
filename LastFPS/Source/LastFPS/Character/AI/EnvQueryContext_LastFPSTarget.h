#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_LastFPSTarget.generated.h"

/**
 * EQS 컨텍스트: "현재 교전 대상(플레이어)".
 *
 * 쿼리어(적 폰)의 AIController 블랙보드에서 TargetActor 키를 읽어 그 액터를 컨텍스트로 제공한다.
 * (없으면 AIController 의 FocusActor 로 폴백.)
 * 카이팅/사격위치 EQS 에서 "타깃으로부터의 거리 / 타깃에 대한 시야" 테스트의 기준점으로 쓴다.
 */
UCLASS()
class LASTFPS_API UEnvQueryContext_LastFPSTarget : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
