#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "Encounter/LastFPSObjectiveHudMode.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSEncounterObjectiveDefinition.generated.h"

class AActor;
class ULastFPSTimedObjectiveComponent;

/**
 * 인카운터 목표 1종의 불변 설정이다.
 *
 * 목표의 "종류"는 이 정의의 파생 클래스가, "위치"는 레벨 배치물이, "언제 쓰이는가"는
 * DT_RoomEncounter 행이 각각 소유한다. 인카운터 런타임은 셋 중 무엇도 알 필요가 없고
 * CreateRuntimeObjective 가 돌려준 컴포넌트만 다룬다.
 *
 * 신규 목표 유형은 이 클래스를 상속해 ConfigureRuntimeObjective 만 구현하면 되며,
 * 런타임·행 구조·서브시스템 중 어느 것도 수정하지 않는다.
 */
UCLASS(Abstract, BlueprintType)
class LASTFPS_API ULastFPSEncounterObjectiveDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * 레벨의 어느 배치물과 짝지어지는가.
	 * 배치물이 이 값과 인카운터 식별자를 Actor Tag 로 함께 들고 있어야 한다
	 * (배리어·스폰 지점이 쓰는 레벨 계약과 동일한 방식).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective")
	FName ObjectiveTag;

	/** 목표 달성까지 필요한 시간(초). 진행 조건은 파생 클래스가 정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective", meta=(ClampMin="1.0", Units="s"))
	float Duration = 60.f;

	/** 진행 누적·갱신 주기(초). 짧을수록 게이지가 부드럽지만 타이머 호출이 늘어난다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective", meta=(ClampMin="0.05", Units="s"))
	float UpdateInterval = 0.25f;

	/**
	 * 성공 시 퀘스트에 통지할 목표 유형.
	 * 런타임 컴포넌트는 이 값을 해석하지 않고 퀘스트 서브시스템으로 그대로 넘긴다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective|Quest")
	ELastFPSObjectiveType QuestObjectiveType = ELastFPSObjectiveType::DefendZone;

	/** 퀘스트 목표의 TargetTag 와 매칭할 구역 태그. 비우면 퀘스트에 통지하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective|Quest")
	FGameplayTag ZoneTag;

	/** HUD 트래커·화면 마커에 표시할 라벨. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective|UI")
	FText DisplayLabel;

	/**
	 * 화면에 어떤 표시를 띄울지. 파생 클래스가 기본값을 정하고, 필요하면 에셋에서 바꾼다.
	 * 표시 슬롯은 하나뿐이라 같은 시점에 두 목표가 화면을 요구하면 뒤쪽이 거부된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Objective|UI")
	ELastFPSObjectiveHudMode HudMode = ELastFPSObjectiveHudMode::None;

	/**
	 * 배치물에 런타임 목표를 만들어 붙인다.
	 * 공통 설정을 적용한 뒤 파생 클래스의 배선(ConfigureRuntimeObjective)을 호출한다.
	 * 서버에서만 호출한다 — 컴포넌트는 복제로 클라이언트에 전달된다.
	 */
	ULastFPSTimedObjectiveComponent* CreateRuntimeObjective(AActor& Anchor) const;

	/** 중앙 비동기 로딩이 미리 준비해야 하는 경로를 넘긴다. */
	virtual void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const {}

	/** 데이터 오류를 초기화 시점에 잡기 위한 검사다. */
	virtual bool IsConfigurationValid(FString& OutFailureReason) const;

protected:
	/**
	 * 목표 종류를 결정하는 참조를 연결한다.
	 * 진행 조건(볼륨)과 실패 감시 대상 중 필요한 것만 채우면 되며, 둘 다 비우면
	 * "시작하면 무조건 진행되고 실패하지 않는" 순수 타이머 목표가 된다.
	 */
	virtual void ConfigureRuntimeObjective(AActor& Anchor, ULastFPSTimedObjectiveComponent& Objective) const {}
};
