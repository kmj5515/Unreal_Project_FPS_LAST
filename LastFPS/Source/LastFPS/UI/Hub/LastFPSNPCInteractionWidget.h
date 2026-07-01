#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "Hub/LastFPSNPCTypes.h"
#include "LastFPSNPCInteractionWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class ULastFPSButtonBase;
class ALastFPSPlayerController;

/**
 * NPC 상호작용 허브 (Menu 레이어).
 * NPC 이름/역할 + 액션별 버튼(대화하기/상점/모듈…)을 동적으로 생성한다.
 * - Menu 입력모드 → 열려 있는 동안 캐릭터 이동 정지
 * - 상점/모듈 등 화면은 이 위젯 위에 스택으로 열리고 닫으면 허브로 복귀
 * - ESC/Back → 닫힘(NativeDestruct) → PlayerController가 캐릭터 카메라로 복귀
 */
UCLASS()
class LASTFPS_API ULastFPSNPCInteractionWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

public:
	/** 허브 구성 — 이름/역할 표시 + 액션 버튼 생성. */
	void Setup(ALastFPSPlayerController* InPC, const FText& InName, const FText& InRole, const TArray<FLastFPSNPCAction>& InActions);

protected:
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** NPC 이름 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Name;

	/** NPC 역할 (옵션) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Role;

	/** 버튼이 채워질 컨테이너 (VerticalBox 등) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UPanelWidget> ButtonContainer;

	/** 액션 버튼으로 생성할 버튼 WBP (ULastFPSButtonBase 계열) */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|NPC")
	TSubclassOf<ULastFPSButtonBase> ActionButtonClass;

private:
	TWeakObjectPtr<ALastFPSPlayerController> OwningPC;
};
