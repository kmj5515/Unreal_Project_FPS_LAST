#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "LastFPSEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;

/**
 * 적(Enemy) 전용 전투 AIController.
 *
 * 책임(단일 책임 원칙):
 *   - 감지: 시야(Sight) 퍼셉션으로 플레이어를 포착해 Blackboard 의 TargetActor 에 기록한다.
 *   - 실행: 빙의 시 BehaviorTree 를 돌린다(의사결정은 전적으로 BT + 데이터 주도).
 *   - 조준: 타깃을 SetFocus 로 바라보게 한다(몸 회전은 Pawn 의 desired rotation 이 처리).
 *
 * 감지 파라미터(시야 반경/시야각 등)는 코드에 박지 않고, 빙의한 Pawn(ALastFPSEnemyCharacter)의
 * ULastFPSAIProfile 에서 읽어 적용한다. 프로파일만 바꿔 감지·공격 특성을 교체할 수 있다.
 *
 * 서버 권위: AIController 는 서버에서만 존재하므로 이 클래스의 로직은 자연히 서버에서만 돈다.
 */
UCLASS()
class LASTFPS_API ALastFPSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALastFPSEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** 에디터에서 지정할 전투 BehaviorTree(BT_Enemy). 비어 있으면 BT 를 돌리지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|Enemy AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** 시야 감지 컴포넌트(컨트롤러 소유 = 두뇌의 감각). */
	UPROPERTY(VisibleAnywhere, Category="LastFPS|Enemy AI")
	TObjectPtr<UAIPerceptionComponent> EnemyPerception;

	UPROPERTY(VisibleAnywhere, Category="LastFPS|Enemy AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

private:
	/** 퍼셉션 갱신 콜백: 플레이어를 포착/소실하면 Blackboard 를 갱신한다. */
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Pawn 의 AIProfile 값으로 시야 설정을 채운다. */
	void ConfigureSightFromProfile();
};
