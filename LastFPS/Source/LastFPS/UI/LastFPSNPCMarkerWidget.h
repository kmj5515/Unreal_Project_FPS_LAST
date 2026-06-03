#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSNPCMarkerWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * NPC 머리 위 3D 플로팅 마커 (WidgetComponent에 할당)
 * - 이름 / 역할 항상 표시
 * - 플레이어가 범위 안에 들어오면 "[F] 대화" 힌트 표시
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

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_NPCName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_NPCRole;

	/** "[F] 대화" 힌트 루트 — Visibility로 전체 토글 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> InteractionHint;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_InteractionLabel;
};
