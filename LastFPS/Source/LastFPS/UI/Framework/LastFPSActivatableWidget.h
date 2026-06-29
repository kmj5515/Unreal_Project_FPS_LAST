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

protected:
	virtual bool NativeOnHandleBackAction() override;

	virtual void NativeOnActivated() override;

	/**
	 * ESC로 닫기 (CommonUI Back 미구성 환경의 폴백).
	 * 메뉴가 열리면 Menu 입력모드라 PC 키바인딩이 안 받으므로, 포커스를 가진
	 * 위젯이 직접 ESC를 받아 자신을 비활성화(pop)한다.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
