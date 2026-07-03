#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LastFPSActivatableWidget.generated.h"

/** 프로젝트 공통 Activatable 베이스 (Modal / Menu / HUD) */
UCLASS(Abstract)
class LASTFPS_API ULastFPSActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	ULastFPSActivatableWidget(const FObjectInitializer& ObjectInitializer);

	/** 메뉴/모달 공통 입력 설정 — Menu 모드 + 커서 표시(NoCapture). */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual bool NativeOnHandleBackAction() override;

	/** BP 직렬화 값이 생성자 설정을 덮을 수 있어 bIsBackHandler=false 를 재차 강제. */
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	/** 닫힐 때(다음 틱) 아래 레이어 최상위 위젯으로 포커스 복원 — 포커스 고아 방지. */
	virtual void NativeOnDeactivated() override;

	/** ESC로 닫기 (CommonUI Back 미구성 환경의 폴백). */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** ESC로 이 위젯을 닫을지. bIsBackHandler(CommonUI Back)와 분리. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
	bool bCloseOnEscape = true;
};
