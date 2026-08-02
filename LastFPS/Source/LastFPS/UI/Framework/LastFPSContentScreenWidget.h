#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSContentScreenWidget.generated.h"

class UWidget;
class UTextBlock;
class ULastFPSButtonBase;

/**
 * 풀스크린 콘텐츠 화면 공용 베이스 (인벤토리 / 임무 / 상점 / 설정 ...).
 * 공통 크롬: 타이틀 텍스트 + 닫기 버튼. 콘텐츠는 파생 WBP가 채운다.
 * 닫기 / Back → 위젯 비활성화(레이어에서 pop).
 *
 * 상단 탭바는 여기 두지 않는다. 탭은 화면이 바뀌어도 유지돼야 하므로 바깥 shell(로비)이 단독으로
 * 소유하고, 이 화면들은 shell 의 스위처 안에서 콘텐츠 역할만 한다.
 */
UCLASS()
class LASTFPS_API ULastFPSContentScreenWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

public:
	/**
	 * 껍데기(허브 메뉴) 안의 탭으로 열렸음을 알린다. 껍데기가 콘텐츠를 만든 직후 호출한다.
	 *
	 * 이 화면 혼자 레이어에 뜰 때는 자기 타이틀바와 닫기 버튼이 필요하지만, 껍데기 안에서는
	 * 그 역할을 상단바가 이미 하고 있어 같은 것이 두 겹으로 보인다. 그래서 자기 크롬을 접는다.
	 *
	 * 어디에 놓였는지는 놓는 쪽만 알 수 있으므로 위젯이 부모를 뒤져 추측하지 않고 통보받는다.
	 */
	void SetHostedInShell(bool bInHostedInShell);

protected:
	virtual void NativeConstruct() override;

	/** 타이틀바 텍스트 (BP에서 화면별 지정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Screen")
	FText ScreenTitle;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_Title;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Close;

	/**
	 * 이 화면이 단독으로 뜰 때만 필요한 크롬(상단바 전체)을 담은 패널.
	 *
	 * WBP 에서 상단바 컨테이너 이름을 이 이름으로 두면 껍데기 안에서 통째로 접힌다.
	 * 이름을 맞추지 않은 화면은 타이틀과 닫기 버튼만 접히고 바의 여백은 남는다.
	 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Panel_ScreenChrome;

private:
	/** 껍데기 안이면 자기 크롬을 접는다. 바인딩이 준비된 뒤에 불러야 한다. */
	void ApplyChromeVisibility();

	UFUNCTION()
	void HandleCloseClicked();

	/** 껍데기 안의 탭으로 열렸는지. 기본은 단독 표시. */
	bool bHostedInShell = false;
};
