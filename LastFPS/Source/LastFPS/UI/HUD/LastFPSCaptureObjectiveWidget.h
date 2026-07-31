#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSCaptureObjectiveWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 점령 목표 표시 — 차오르는 게이지가 주된 정보다.
 *
 * 방어와 달리 진행이 플레이어 위치에 종속돼 멈췄다 재개되므로, 남은 시간보다
 * "얼마나 찼는가"가 읽기 쉽다. 진행이 멈춘 상태는 색으로 구분해 알린다.
 *
 * WBP 계약: 아래 이름의 위젯을 만들면 자동 바인딩된다. 없으면 그 항목만 표시하지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSCaptureObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 목표 라벨과 표시를 초기화한다. 표시 시작 시 1회 호출한다. */
	void SetupObjective(const FText& InLabel);

	/** 0~1 진행률. 직전 값보다 커지지 않았으면 정체 상태로 본다. */
	void UpdateProgress(float Progress01);

protected:
	virtual void NativeConstruct() override;

	/** 목표 이름 (예: "전초기지 점령"). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label;

	/** 점령 게이지. 이 위젯의 핵심 정보다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Capture;

	/** 백분율 텍스트 (예: "62%"). 선택 항목. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Percent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FLinearColor CaptureFillColor = FLinearColor(0.2f, 0.9f, 0.4f, 1.f);

	/** 구역을 벗어나 진행이 멈춘 동안 쓰는 색이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	FLinearColor StalledFillColor = FLinearColor(0.7f, 0.7f, 0.2f, 1.f);

	/**
	 * 진행이 멈췄다고 볼 때까지 기다리는 시간(초).
	 * 갱신 주기 사이에는 값이 그대로라 즉시 판단하면 정상 진행 중에도 깜빡인다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture", meta=(ClampMin="0.05", Units="s"))
	float StallDetectDelay = 0.6f;

	/**
	 * BP 추가 연출 훅.
	 * 파라미터 이름은 멤버(bStalled)와 겹치지 않게 둔다 — UHT 가 생성하는 스텁에서
	 * 멤버를 가려 C4458 경고가 나고, 이 프로젝트는 경고를 에러로 취급한다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Capture")
	void OnCaptureDisplayUpdated(float Progress01, bool bIsStalled);

private:
	void ApplyStalled(bool bNewStalled);

	float LastProgress = 0.f;
	double LastAdvanceTime = 0.0;
	bool bStalled = false;
};
