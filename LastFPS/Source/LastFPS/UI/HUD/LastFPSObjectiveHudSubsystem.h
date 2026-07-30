#pragma once

#include "CoreMinimal.h"
#include "Encounter/LastFPSObjectiveHudMode.h"
#include "Subsystems/WorldSubsystem.h"
#include "LastFPSObjectiveHudSubsystem.generated.h"

/**
 * 활성 표시가 바뀌었다. Mode 가 None 이면 아무것도 표시하지 않는다.
 * Source 는 표시를 요청한 주체(목표 컴포넌트, 보스 프레젠터 등)이며,
 * HUD 가 여기서 진행률·체력 같은 실제 값을 읽어 간다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLastFPSObjectiveHudChanged,
	ELastFPSObjectiveHudMode, Mode,
	UObject*, Source);

/**
 * 목표 표시 슬롯 하나를 소유하는 라우터다.
 *
 * 점령·방어·보스는 동시에 띄우지 않는다는 규칙이 있어 슬롯을 하나만 둔다.
 * 표시를 원하는 쪽이 요청하고, 끝나면 반납한다. HUD 는 어떤 목표가 있는지 몰라도 되고
 * 이 델리게이트 하나만 구독하면 된다 — 새 목표 유형이 생겨도 HUD 배선은 그대로다.
 *
 * 클라이언트 로컬 표시 전용이라 복제하지 않는다. 각 머신이 자기 화면을 소유한다.
 */
UCLASS()
class LASTFPS_API ULastFPSObjectiveHudSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 표시 슬롯을 점유한다.
	 * 이미 다른 주체가 점유 중이면 거부하고 경고를 남긴다 — 동시 표시는 설계상 허용하지 않으므로
	 * 조용히 덮어쓰면 어느 쪽이 사라졌는지 추적할 수 없기 때문이다.
	 * 같은 주체가 다시 요청하면 모드만 갱신한다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|HUD")
	bool RequestPresentation(ELastFPSObjectiveHudMode Mode, UObject* Source);

	/** 슬롯을 반납한다. 점유 중인 주체가 아니면 무시한다(늦게 도착한 해제로 남의 표시를 끄지 않는다). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|HUD")
	void ReleasePresentation(UObject* Source);

	UFUNCTION(BlueprintPure, Category="LastFPS|HUD")
	ELastFPSObjectiveHudMode GetActiveMode() const { return ActiveMode; }

	/** 현재 표시 주체. 파괴됐으면 null. */
	UFUNCTION(BlueprintPure, Category="LastFPS|HUD")
	UObject* GetActiveSource() const { return ActiveSource.Get(); }

	UPROPERTY(BlueprintAssignable, Category="LastFPS|HUD")
	FOnLastFPSObjectiveHudChanged OnPresentationChanged;

	virtual void Deinitialize() override;

private:
	/** 주체가 파괴됐는데 반납하지 않은 경우를 정리한다. 정리했으면 true. */
	bool PruneDeadSource();

	void Broadcast();

	ELastFPSObjectiveHudMode ActiveMode = ELastFPSObjectiveHudMode::None;

	/** 표시 주체는 레벨 액터의 컴포넌트일 수 있어 약참조로 든다. */
	TWeakObjectPtr<UObject> ActiveSource;
};
