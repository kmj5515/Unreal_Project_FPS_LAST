#pragma once

#include "UI/Framework/LastFPSModalDialogBase.h"
#include "UI/Result/LastFPSMissionResultTypes.h"
#include "LastFPSMissionResultWidget.generated.h"

class UImage;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UWrapBox;
class ULastFPSButtonBase;
class ULastFPSItemSlotWidget;
class ULastFPSStatEntryWidget;

/**
 * WBP_MissionResult 의 Parent — 던전/임무 클리어 결과 화면.
 *
 * FLastFPSMissionResult 만 소비한다. 퀘스트·플레이어 상태·재화 시스템을 알지 않으므로
 * 결과를 만들어 주는 쪽이 바뀌어도 이 화면은 영향을 받지 않는다.
 *
 * 아직 산출 시스템이 없는 구획(점수)은 계약 필드가 비어 있으면 스스로 접는다.
 * 표시 문구는 코드에 박지 않고 WBP 에서 저작하도록 EditDefaultsOnly 로 노출한다.
 */
UCLASS()
class LASTFPS_API ULastFPSMissionResultWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	/**
	 * 결과 팝업을 띄운다. 태그와 설정 절차를 이 클래스가 소유하므로 호출부는 결과값만 넘긴다.
	 * @return 열린 팝업. 카탈로그 미등록·레이아웃 미준비면 nullptr
	 */
	static ULastFPSMissionResultWidget* ShowPopup(
		const UObject* WorldContext,
		const FLastFPSMissionResult& InResult);

	void SetupResult(const FLastFPSMissionResult& InResult);

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnHandleBackAction() override;

	/** 임무 이름 / 클리어 시간 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_MissionName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_ElapsedTime;
	
	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> In_Animation;
	
	/** 재화 보상 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Credits;

	/** 사용 캐릭터 초상 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Portrait;

	/** 점수 구획 — 산출 시스템이 없으면 통째로 접힌다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_Score;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Score;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Score;

	/** 전투 통계 행이 채워질 패널 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UPanelWidget> Box_Stats;

	/** 획득 아이템이 채워질 컨테이너 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> WrapBox_Items;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Confirm;

	/** 통계 행 1개를 만들 위젯 클래스 (WBP_StatEntry). 지정하지 않으면 통계를 채울 수 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult")
	TSubclassOf<ULastFPSStatEntryWidget> StatEntryClass;

	/** 획득 아이템 1칸을 그릴 위젯 클래스 (WBP_ItemSlot 계열) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult")
	TSubclassOf<ULastFPSItemSlotWidget> ItemSlotWidgetClass;

	/**
	 * 통계 행 라벨.
	 * 문구를 코드에 박으면 화면마다 다른 표기를 쓸 수 없어 WBP 에서 저작하게 둔다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult|Label")
	FText DamageDealtLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult|Label")
	FText DamageTakenLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult|Label")
	FText KillsLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult|Label")
	FText DeathsLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MissionResult|Label")
	FText AssistsLabel;

private:
	void HandleConfirmClicked();

	void RefreshHeader(const FLastFPSMissionResult& InResult);
	void RefreshScore(const FLastFPSMissionResult& InResult);
	void RefreshCombatStats(const FLastFPSMissionCombatStats& InStats);
	void RefreshItems(const TArray<FLastFPSItemGrant>& InItems);

	/**
	 * 통계 행 1개를 Box_Stats 에 추가한다.
	 * @param InLabel 비어 있으면 WBP 저작 누락이므로 행을 만들지 않는다.
	 */
	void AddStatRow(const FText& InLabel, const FText& InValue);
};
