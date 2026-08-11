#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSQuestEntryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestSelectedSignature, FName, QuestId);

class UTextBlock;
class UImage;
class UButton;
class ULastFPSQuestSubsystem;

/**
 * WBP_QuestEntry 의 Parent — 퀘스트 목록의 한 줄.
 * Designer(모두 선택): TB_Title / TB_Summary / TB_Status / TB_Reward / TB_Progress / Img_Icon / Btn_Claim.
 * 정적 정의는 행에서, 런타임 상태·진행·수령가능은 QuestSubsystem 에서 읽는다.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 정적 정의(행) + 런타임 상태(서브시스템)로 표시 내용 채우기. Btn_Claim → TryClaimReward 배선. */
	void SetupQuest(ULastFPSQuestSubsystem* InSubsystem, FName InQuestId, const FLastFPSQuestData& InQuest);

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest")
	FOnQuestSelectedSignature OnQuestSelected;

protected:
	virtual void NativeDestruct() override;

	/** 상태 텍스트/색상 등 BP 측 스타일링 훅 (런타임 상태 전달) */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnQuestDisplayed(ELastFPSQuestType Type, ELastFPSQuestStatus Status);

	/** 보상 수령 버튼 클릭 → 서브시스템에 수령 요청 */
	UFUNCTION()
	void HandleClaimClicked();

	UFUNCTION()
	void HandleSelectClicked();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Btn_Select;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Location;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Distance;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Summary;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Status;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Reward;

	/** 목표 진행도 표시 ("2/3" 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Progress;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

	/** 완료 시에만 활성/표시되는 보상 수령 버튼 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Btn_Claim;

private:
	TWeakObjectPtr<ULastFPSQuestSubsystem> OwningSubsystem;
	FName BoundQuestId;

public:
	/** 상태 enum → 표시 텍스트 (목록/HUD 공용) */
	static FText StatusToText(ELastFPSQuestStatus Status);

	/** 미수락 퀘스트에는 상태 대신 의뢰인 안내를 포함해 목록과 상세 화면의 표현을 통일한다. */
	static FText BuildStatusText(
		const ULastFPSQuestSubsystem* QuestSubsystem,
		FName QuestId,
		ELastFPSQuestStatus Status);
};
