#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSNPCMarkerWidget.generated.h"

class UTextBlock;
class UImage;
class UProgressBar;
class UMaterialInstanceDynamic;

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
	 * - Img_HoldGauge 의 머티리얼(MI_Progress_Radial 등) 스칼라 파라미터(GaugeProgressParam)에 진행도 설정 (C++)
	 * - PB_HoldGauge(선택)에 Percent 설정
	 * - 추가 연출이 필요하면 OnInteractionProgressChanged(BP 이벤트)로도 전달
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	void SetInteractionProgress(float Progress);

protected:
	virtual void NativeConstruct() override;

	/** (선택) 진행도 표시 추가 연출 훅 — 머티리얼 구동은 C++가 하므로 보통 불필요 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|NPC")
	void OnInteractionProgressChanged(float Progress);

	/** 원형 게이지 이미지 — 브러시에 MI_Progress_Radial 지정 시 C++가 동적 인스턴스로 진행도 구동 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_HoldGauge;

	/** 게이지 머티리얼의 진행도 스칼라 파라미터 이름 (M_Progress_Radial = "Progress") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FName GaugeProgressParam = TEXT("Progress");

	/** 채우는 방향 반전 — MI_Progress_Radial은 값이 커질수록 흰색이 비워져, 채워지게 보이려면 반전(1-Progress)이 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	bool bInvertGaugeFill = true;

	/** Img_HoldGauge 브러시에서 만든 동적 머티리얼 인스턴스 (런타임) */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GaugeMID;

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
