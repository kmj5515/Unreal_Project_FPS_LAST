#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "UI/Theme/LastFPSUIThemeReceiver.h"
#include "LastFPSStatSummaryWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSStatEntryWidget;

/**
 * WBP_StatSummary 의 Parent — 장착 장비 합산 스탯 요약.
 *
 * 표시 전용이다. 어떤 스탯이 있는지는 ELastFPSEquipmentStat 이 정하고, 이 위젯은 넘어온 합계를
 * 행 위젯 목록으로 옮긴다. 스탯이 늘어도 이 클래스는 수정할 필요가 없다.
 *
 * 행의 배치·서식은 ULastFPSStatEntryWidget 이 책임지므로 여기서는 개수와 순서만 다룬다.
 */
UCLASS()
class LASTFPS_API ULastFPSStatSummaryWidget : public UUserWidget, public ILastFPSUIThemeReceiver
{
	GENERATED_BODY()

public:
	/** 값이 0 인 스탯은 생략해 목록이 길어지지 않게 한다. */
	void SetTotals(const FLastFPSEquipmentStatTotals& InTotals);

	virtual void ApplyUITheme(const ULastFPSUIThemeAsset& Theme) override;

protected:
	/** 스탯 행 1개를 만들 위젯 클래스. 지정하지 않으면 목록을 채울 수 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TSubclassOf<ULastFPSStatEntryWidget> StatEntryClass;

	/** 스탯 행이 채워질 패널 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) 
	TObjectPtr<UPanelWidget> Box_Stats;
	/** 보정이 하나도 없을 때 안내 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Empty;

	/**
	 * 보정이 하나도 없을 때 TB_Empty 에 넣을 문구.
	 * 문구를 코드에 박으면 화면마다 다른 말을 쓸 수 없어 WBP 에서 저작하게 둔다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	FText EmptyText;

private:
	/**
	 * 필요한 개수만큼 행 위젯을 확보한다.
	 * 요약은 장비를 바꿀 때마다 갱신되므로 매번 파괴·생성하지 않고 만들어 둔 행을 다시 쓴다.
	 * @return 준비된 행 개수(요청 수보다 적으면 생성에 실패한 것)
	 */
	int32 EnsureEntryCount(int32 RequiredCount);

	/** 생성 순서대로 보관한다. 인덱스가 곧 Box_Stats 안의 표시 순서다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULastFPSStatEntryWidget>> EntryWidgets;

	/**
	 * 화면 진입 시 받은 테마. 행 위젯은 그 뒤에 만들어지므로 여기 보관했다가 생성 직후 적용한다.
	 * 테마 에셋의 수명은 설정이 쥐고 있어 이 위젯이 소유권을 가질 이유가 없다.
	 */
	TWeakObjectPtr<const ULastFPSUIThemeAsset> CachedTheme;

	/** 설정 누락 경고를 갱신마다 반복하지 않기 위한 표시 */
	bool bLoggedMissingSetup = false;
};
