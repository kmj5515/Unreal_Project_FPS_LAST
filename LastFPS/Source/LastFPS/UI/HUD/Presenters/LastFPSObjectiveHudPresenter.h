#pragma once

#include "CoreMinimal.h"
#include "Encounter/LastFPSObjectiveHudMode.h"
#include "UObject/Object.h"
#include "LastFPSObjectiveHudPresenter.generated.h"

class ULastFPSCaptureObjectiveWidget;
class ULastFPSDefendObjectiveWidget;
class ULastFPSTimedObjectiveComponent;
class UWidget;

/**
 * 목표 표시 슬롯을 실제 위젯으로 연결한다.
 *
 * 라우터(ULastFPSObjectiveHudSubsystem)가 "지금 무엇을 켜야 하는가"만 알려주고,
 * 여기서 모드별 위젯을 보이고 숨긴다. 표시 슬롯이 하나뿐이라 항상 하나만 켜진다.
 *
 * 새 목표 유형이 생기면 모드 enum 과 위젯을 추가하고 이 프레젠터의 스위치만 늘리면 되며,
 * HUD View 와 목표 컴포넌트는 손대지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSObjectiveHudPresenter final : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 모드별 위젯(모두 null 허용)을 받아 전부 숨긴 상태로 시작한다.
	 *
	 * 보스 체력바는 여기서 다루지 않는다 — 그 위젯은 추적 대상의 수명에 맞춰 스스로
	 * 보이고 숨는 로직을 이미 갖고 있어, 여기서 가시성을 함께 만지면 주인이 둘이 된다.
	 * 보스는 라우터에서 "슬롯 점유 허가"만 받고 표시 자체는 계속 자기가 소유한다.
	 */
	void Initialize(
		ULastFPSDefendObjectiveWidget* InDefendWidget,
		ULastFPSCaptureObjectiveWidget* InCaptureWidget);

	/** 라우터 구독을 시작한다. 월드가 준비된 뒤 호출한다. 중복 구독은 무시한다. */
	void BindToWorld(UWorld* World);
	void Unbind();

	/** 활성 목표의 진행·체력을 화면에 반영한다. HUD View 의 틱에서 호출한다. */
	void Tick(float DeltaTime);

private:
	UFUNCTION()
	void HandlePresentationChanged(ELastFPSObjectiveHudMode Mode, UObject* Source);

	/** 모드에 해당하는 위젯만 남기고 나머지를 숨긴다. */
	void ApplyMode(ELastFPSObjectiveHudMode Mode);

	static void SetWidgetVisible(UWidget* Widget, bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSDefendObjectiveWidget> DefendWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSCaptureObjectiveWidget> CaptureWidget;

	/** 현재 표시 중인 목표 컴포넌트. 보스 모드에서는 null 이다. */
	TWeakObjectPtr<ULastFPSTimedObjectiveComponent> ActiveObjective;

	TWeakObjectPtr<UWorld> BoundWorld;
	ELastFPSObjectiveHudMode ActiveMode = ELastFPSObjectiveHudMode::None;
};
