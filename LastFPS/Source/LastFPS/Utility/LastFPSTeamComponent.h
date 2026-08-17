#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Utility/LastFPSTeamTypes.h"
#include "LastFPSTeamComponent.generated.h"

/**
 * 진영을 스스로 밝히지 못하는 액터에 진영을 부여한다.
 *
 * 폰은 컨트롤러가 IGenericTeamAgentInterface 를 구현해 진영을 알리지만, 수호 목표(리액터)나
 * 파괴물처럼 컨트롤러가 없는 액터는 그 경로가 없어 "진영 미상"이 되고,
 * 피해 판정이 미상을 아군으로 보지 않으므로 아군 사격에 그대로 노출된다.
 *
 * 이 컴포넌트를 붙이면 레벨·블루프린트에서 진영을 지정할 수 있고,
 * 피해 판정 코드는 대상 종류를 열거하지 않고 기존 진영 계약만으로 처리한다.
 *
 * 값은 저작 시점에 정해지는 설정이라 복제하지 않는다. 서버와 클라이언트가 같은 기본값을 읽는다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSTeamComponent : public UActorComponent, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ULastFPSTeamComponent();

	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

protected:
	/** 이 액터가 속한 진영. 수호 목표는 플레이어와 같은 진영이어야 적만 파괴할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Team")
	ELastFPSTeam Team = ELastFPSTeam::Player;
};
