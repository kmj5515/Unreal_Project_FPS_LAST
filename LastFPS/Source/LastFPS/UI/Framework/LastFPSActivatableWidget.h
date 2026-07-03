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

	/**
	 * 메뉴/모달 공통 입력 설정 — Menu 모드 + 커서 표시(NoCapture).
	 * 선언하지 않으면 모달(수량 다이얼로그 등)이 닫힐 때 CommonUI가 하위 스택
	 * (HUD=Game 모드)의 config를 재적용해 커서가 사라진다. (HUD는 자체 오버라이드로 Game 유지)
	 */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual bool NativeOnHandleBackAction() override;

	virtual void NativeOnActivated() override;

	/**
	 * 이 위젯이 닫히면(특히 Modal 레이어) 다음 틱에 아래 레이어 최상위 위젯으로 포커스를 되돌린다.
	 * 모달은 화면과 다른 레이어라 닫혀도 아래 화면의 NativeOnActivated가 재실행되지 않아
	 * 수동 SetKeyboardFocus가 다시 걸리지 않는다 → 포커스 고아 → ESC가 PC로 새는 문제를 보완.
	 */
	virtual void NativeOnDeactivated() override;

	/**
	 * ESC로 닫기 (CommonUI Back 미구성 환경의 폴백).
	 * 메뉴가 열리면 Menu 입력모드라 PC 키바인딩이 안 받으므로, 포커스를 가진
	 * 위젯이 직접 ESC를 받아 자신을 비활성화(pop)한다.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
