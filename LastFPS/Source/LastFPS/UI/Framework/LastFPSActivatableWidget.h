#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LastFPSActivatableWidget.generated.h"

class USoundBase;

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
	/** Back/ESC 단일 종료 지점. 모달은 오버라이드해 결과를 브로드캐스트한 뒤 닫는다. */
	virtual bool NativeOnHandleBackAction() override;

	/** BP 직렬화 값이 생성자 설정을 덮을 수 있어 bIsBackHandler=false 를 재차 강제. */
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	/** 닫힐 때(다음 틱) 아래 레이어 최상위 위젯으로 포커스 복원 — 포커스 고아 방지. */
	virtual void NativeOnDeactivated() override;

	/**
	 * ESC 로 닫기.
	 *
	 * CommonUI Back 액션은 쓰지 않는다. 이 프로젝트는 입력 config 의 단일 writer 가
	 * PlayerController(ApplyInputConfigForMenuState)라 bEnableDefaultInputConfig 가 꺼져 있고,
	 * 그 상태에서는 Back 라우팅이 활성화되지 않는다. 그래서 raw ESC 로 직접 받는다.
	 */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** ESC 로 이 위젯을 닫을지. 셸 안 탭 콘텐츠처럼 스스로 닫히면 안 되는 화면은 끈다. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI")
	bool bCloseOnEscape = true;

	/** 활성화(열림) 시 재생할 UI 사운드 (비우면 무음). HUD 등 무음 위젯은 비워 둠. */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> ActivateSound;

	/** 비활성화(닫힘) 시 재생할 UI 사운드 (비우면 무음). */
	UPROPERTY(EditDefaultsOnly, Category="LastFPS|UI|Sound")
	TObjectPtr<USoundBase> DeactivateSound;
};
