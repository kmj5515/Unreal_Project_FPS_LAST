#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSEncounterObjectiveDefinition.h"
#include "LastFPSDefendObjectiveDefinition.generated.h"

class UGameplayEffect;

/**
 * 수호(방어) 목표의 설정이다 — "정해진 시간 버티되, 지킬 대상이 파괴되면 실패".
 *
 * 진행 조건을 걸지 않으므로 시작하면 시간이 계속 흐르고, 배치물(지킬 장치) 자신을
 * 실패 감시 대상으로 연결한다. 장치 체력은 GAS 어트리뷰트라 초기화 효과로 지정한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSDefendObjectiveDefinition : public ULastFPSEncounterObjectiveDefinition
{
	GENERATED_BODY()

public:
	ULastFPSDefendObjectiveDefinition();

	/**
	 * 지킬 대상의 최대 체력.
	 * 초기화 효과를 지정하지 않으면 이 값으로 Health/MaxHealth 를 직접 세팅한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Defend", meta=(ClampMin="1.0"))
	float DeviceMaxHealth = 8000.f;

	/**
	 * 지킬 대상의 체력을 세팅하는 초기화 효과(선택).
	 * 지정하면 DeviceMaxHealth 대신 이 효과를 적용한다 — 저항·감쇠 같은 부가 스탯이
	 * 필요할 때 쓴다. 목표 재시작 시에도 같은 방식으로 복구한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Defend")
	TSubclassOf<class UGameplayEffect> DeviceInitEffect;

	virtual bool IsConfigurationValid(FString& OutFailureReason) const override;

protected:
	/** 배치물 자신이 지킬 대상이다 — 파괴되면 실패한다. 진행 조건은 걸지 않는다. */
	virtual void ConfigureRuntimeObjective(AActor& Anchor, ULastFPSTimedObjectiveComponent& Objective) const override;
};
