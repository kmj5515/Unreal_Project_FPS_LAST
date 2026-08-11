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

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleTrackClicked();

	UFUNCTION()
	void HandleClaimClicked();

	/**
	 * 퀘스트 상태 변경 통지 — 추적은 동시에 1건이라 다른 퀘스트를 추적하면 이 퀘스트의
	 * 버튼 표시도 함께 바뀌어야 한다. 클릭 직후 낙관적 갱신 대신 이 통지만 신뢰한다.
	 */
	UFUNCTION()
	void HandleQuestStateChanged();

	/** 트래킹 상태 변경 시 BP 측 버튼/텍스트 스타일링 훅 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnQuestTrackStateChanged(bool bIsTracked);

private:
	/** 구조화 보상(크레딧 + 아이템)을 채운다. 아이템 정의는 EquipmentSubsystem 에서 조회. */
	void RefreshRewardSection(const FLastFPSQuestReward& InReward, const FText& InFallbackText);

	/**
	 * 상태에 따라 바뀌는 표시(상태 문구·현재 목표·진행률·버튼)를 다시 그린다.
	 * 선택 시점과 상태 변경 통지가 같은 코드를 쓰도록 분리했다 — 제목/배너/보상 같은
	 * 정적 내용은 SetupQuest 가 소유하므로 여기서 건드리지 않는다.
	 */
	void RefreshRuntimeState();

	/** 상태 변경 구독을 지금 표시 중인 서브시스템으로 옮긴다(이전 구독은 반드시 해제). */
	void BindStateChanged(ULastFPSQuestSubsystem* InSubsystem);

	/** 상태 변경 구독 해제 — 위젯 파괴/대상 교체 시 호출. */
	void UnbindStateChanged();

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

	/** 기존 위젯 레이아웃 호환용 자리. 퀘스트 일지에서는 직접 수락을 제공하지 않아 항상 숨긴다. */
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
