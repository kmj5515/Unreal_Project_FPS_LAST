#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/LastFPSCharacterTypes.h"
#include "LastFPSAIProfile.generated.h"

class AAIController;

/**
 * 적 AI의 행동 파라미터를 담는 데이터 에셋.
 * 컨트롤러/BT 노드는 코드에 값을 박지 않고 이 프로파일에서 읽어 동작한다(데이터 주도).
 * 공격 종류는 Behavior Tree의 공격 Task가 결정한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSAIProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	ELastFPSAIBehaviorType BehaviorType = ELastFPSAIBehaviorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	TSubclassOf<AAIController> AIControllerClass;

	/** 플레이어를 감지(시야에 포착)하는 최대 거리(cm). 시야 반경으로도 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float DetectionRange = 1200.f;

	/**
	 * 감지한 타깃을 놓치는 거리(cm). 이 거리를 벗어나면 타깃을 해제한다.
	 * 감지/해제 거리를 다르게 둬 추격 히스테리시스를 만든다(추격이 덜 끊김).
	 * 0 이하면 DetectionRange 를 그대로 쓴다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float LoseSightRange = 1600.f;

	/**
	 * 카이팅: 타깃이 이 거리보다 가까워지면 뒤로/옆으로 빠져 거리를 유지한다(cm).
	 * 0 이하면 카이팅 안 함(제자리 사격). 보통 AttributeSet의 AttackRange보다 작게 둔다.
	 * 원거리 적 전용 — 근접 적은 0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float KeepDistance = 0.f;

	/** 공격 후 다음 판단까지의 최소 간격(초). BT 공격 태스크의 페이싱에 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float ReactionDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	bool bCanAttack = true;

};
