#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSNPCMarkerWidget.generated.h"

class UTextBlock;
class UImage;
class UProgressBar;

/**
 * NPC 머리 위 3D 플로팅 마커 (WidgetComponent에 할당)
 * - 이름 / 역할 항상 표시
 * - 플레이어가 범위 안에 들어오면 "[G] 대화" 힌트 표시
 */
UCLASS()
class LASTFPS_API ULastFPSNPCMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** NPC 이름과 역할 텍스트 초기화 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	void SetNPCInfo(const FText& InName, const FText& InRole);

	/** 플레이어 범위 진입/이탈 시 힌트 표시 전환 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	void SetInteractionHintVisible(bool bVisible);

	/** 행동 텍스트 교체 ("대화", "상점" 등) */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	void SetInteractionLabel(const FText& InLabel);

	/**
	 * 홀드 진행도(0~1)를 게이지에 반영. 0이면 게이지 숨김.
	 * - PB_HoldGauge(선택)에 Percent 설정
	 * - 원형 게이지는 OnInteractionProgressChanged(BP 이벤트)에서 머티리얼 파라미터로 구성
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	void SetInteractionProgress(float Progress);

protected:
	/** 진행도 표시 훅 — 디자이너가 원형 게이지(머티리얼/리테이너 박스)를 진행도로 구성 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|NPC")
	void OnInteractionProgressChanged(float Progress);
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_NPCName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_NPCRole;

	/** "[G] 대화" 힌트 루트 — Visibility로 전체 토글 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> InteractionHint;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_InteractionLabel;

	/** 홀드 게이지 루트 — 진행도 0이면 Collapsed (원형 게이지 컨테이너) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> HoldGaugeRoot;

	/** 선택적 진행 바 — 원형 머티리얼 대신 단순 바를 쓸 경우 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_HoldGauge;
};
