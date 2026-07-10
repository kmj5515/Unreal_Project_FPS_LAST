#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Character/LastFPSCharacterTypes.h"
#include "LastFPSAIProfile.generated.h"

class AAIController;

/**
 * 적 AI의 행동 파라미터를 담는 데이터 에셋.
 * 컨트롤러/BT 노드는 코드에 값을 박지 않고 이 프로파일에서 읽어 동작한다(데이터 주도).
 * 근접/원거리 구분도 AttackAbilityTag 로 지정한 GAS 어빌리티에 위임한다.
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

	/** 이 거리 안에 들어오면 이동을 멈추고 공격을 시도한다(cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float AttackRange = 200.f;

	/** 공격 후 다음 판단까지의 최소 간격(초). BT 공격 태스크의 페이싱에 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin=0))
	float ReactionDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	bool bCanAttack = false;

	/**
	 * 공격 시 발동할 GAS 어빌리티 태그(예: Ability.Enemy.MeleeSwing / Ability.Enemy.Shoot).
	 * 이 태그를 AssetTags 로 가진 어빌리티가 캐릭터 AbilitySet 에 부여돼 있어야 한다.
	 * 근접이냐 원거리냐는 어빌리티 구현이 결정하므로, 프로파일만 바꿔 공격 방식을 교체할 수 있다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(EditCondition="bCanAttack"))
	FGameplayTag AttackAbilityTag;
};
