#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSDefendObjectiveWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 방어 목표 표시 — 남은 시간이 주된 정보다.
 *
 * 점령과 달리 플레이어가 게이지를 "채우는" 감각이 아니라 "버티는" 감각이라
 * 남은 시간을 숫자로 보여 준다. 지킬 대상의 체력은 보조 정보로 함께 노출한다.
 *
 * WBP 계약: 아래 이름의 위젯을 만들면 자동 바인딩된다. 없으면 그 항목만 표시하지 않는다.
 */
UCLASS()
class LASTFPS_API ULastFPSDefendObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 목표 라벨과 표시를 초기화한다. 표시 시작 시 1회 호출한다. */
	void SetupObjective(const FText& InLabel, float InTotalSeconds);

	/**
	 * 남은 시간과 지킬 대상 체력을 한 번에 갱신한다.
	 * 둘을 따로 받으면 BP 훅이 직전 프레임 값을 섞어 받게 되므로 함께 넘긴다.
	 * Health01 이 음수면 체력 표시를 숨긴다.
	 */
	void UpdateDisplay(float Progress01, float Health01);

protected:
	virtual void NativeConstruct() override;

	/** 목표 이름 (예: "반응로 코어 사수"). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Label;

	/** 남은 시간 (예: "01:12"). 이 위젯의 핵심 정보다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RemainingTime;

	/** 지킬 대상 체력 게이지 (보조). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_TargetHealth;

	/** 대상 체력이 이 비율 아래로 떨어지면 경고색으로 바꾼다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Defend", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LowHealthThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Defend")
	FLinearColor HealthFillColor = FLinearColor(0.15f, 0.75f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Defend")
	FLinearColor HealthLowFillColor = FLinearColor(1.f, 0.25f, 0.15f, 1.f);

	/** 표시를 갱신할 때 BP 에서 추가 연출을 붙이는 훅이다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Defend")
	void OnDefendDisplayUpdated(float RemainingSeconds, float Health01);

private:
	/** 남은 초를 mm:ss 로 만든다. */
	static FText FormatRemaining(float RemainingSeconds);

	void ApplyTargetHealth(float Health01);

	float TotalSeconds = 0.f;
};
