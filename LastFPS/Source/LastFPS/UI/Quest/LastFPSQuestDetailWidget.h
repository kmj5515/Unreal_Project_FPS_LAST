#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSQuestDetailWidget.generated.h"

class ULastFPSButtonBase;
class UImage;
class UTextBlock;
class UWidget;
class UWrapBox;
class ULastFPSQuestSubsystem;
class ULastFPSItemSlotWidget;

/**
 * 퀘스트 상세 정보 패널 위젯
 */
UCLASS()
class LASTFPS_API ULastFPSQuestDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 선택된 퀘스트 정보로 상세 패널을 갱신합니다. */
	void SetupQuest(ULastFPSQuestSubsystem* InSubsystem, FName InQuestId, const FLastFPSQuestData& InQuest);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 버튼 클릭 핸들러 */
	UFUNCTION()
	void HandleAcceptClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleTrackClicked();

	UFUNCTION()
	void HandleClaimClicked();

	/** 트래킹 상태 변경 시 BP 측 버튼/텍스트 스타일링 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnQuestTrackStateChanged(bool bIsTracked);

private:
	/** 구조화 보상(크레딧 + 아이템)을 채운다. 아이템 정의는 EquipmentSubsystem 에서 조회. */
	void RefreshRewardSection(const FLastFPSQuestReward& InReward, const FText& InFallbackText);

protected:

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_RecommendedLevel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Banner;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Progress_txt;

	/** 현재 목표가 없을 때 아이콘과 빈 여백까지 함께 접는 행 컨테이너 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Box_ObjectiveMarker;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_LabelReward;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Description;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CurrentObjective;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Status;

	/** 구조화 보상이 없을 때만 쓰는 폴백 문자열(FLastFPSQuestData::RewardText) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Reward;

	/** 보상 크레딧 수치 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_RewardCredits;

	/** 보상 아이템 아이콘을 가로로 나열할 컨테이너 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> WrapBox_RewardItems;

	/**
	 * 보상 아이템 1칸을 그릴 위젯 클래스 (WBP_ItemSlot 계열).
	 * 인벤토리 슬롯과 동일한 표현을 재사용하려고 전용 클래스를 새로 만들지 않았다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSubclassOf<ULastFPSItemSlotWidget> RewardSlotWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Progress;

	/** 상호작용 버튼들 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Btn_Accept;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Btn_Cancel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Btn_Track;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Btn_Claim;

private:
	TWeakObjectPtr<ULastFPSQuestSubsystem> OwningSubsystem;
	FName BoundQuestId;
};
