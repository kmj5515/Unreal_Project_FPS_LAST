#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/LastFPSQuestMarkerTypes.h"
#include "LastFPSQuestNPCMarkerWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;

/**
 * NPC 머리 위 퀘스트 마커 위젯 — 심볼(수락 가능/대화 목표)과 강조 색을 반영한다.
 * 표시 여부와 심볼 결정은 퀘스트 서브시스템이 소유하고, 이 위젯은 받은 상태를 그리기만 한다.
 *
 * 색은 배경 다이아몬드 머티리얼의 그라디언트 파라미터로 넣는다 — UMG 틴트는 단색이라
 * 그라디언트를 표현할 수 없기 때문이다. 머티리얼이 없으면 단색 틴트로 자동 폴백한다.
 * WBP 계약: Img_Background(다이아몬드), Img_Symbol(느낌표/물음표) — 둘 다 선택 바인딩.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestNPCMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 마커 컴포넌트가 상태 변경 시 호출 — 심볼 브러시와 색을 갱신한다. */
	void ApplyMarkerState(ELastFPSQuestNPCMarkerSymbol Symbol, bool bTracked);

protected:
	virtual void NativeConstruct() override;

	/** 추가 연출 훅 (스케일 펄스 등) — 색·심볼 갱신은 C++ 가 담당한다. */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Quest")
	void OnMarkerStateChanged(ELastFPSQuestNPCMarkerSymbol Symbol, bool bTracked);

	/** 다이아몬드 배경 — 브러시에 그라디언트 머티리얼을 지정하면 C++ 가 동적 인스턴스로 구동한다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Background;

	/** 심볼 이미지 — 아래 브러시 중 상태에 맞는 것으로 교체된다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> Img_Symbol;

	/** 수락 가능(느낌표) 심볼 브러시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Symbol")
	FSlateBrush AvailableSymbolBrush;
	
	/** 대화 목표(물음표) 심볼 브러시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Symbol")
	FSlateBrush ObjectiveSymbolBrush;

	/** 진행 중(모래시계 등) 심볼 브러시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Symbol")
	FSlateBrush InProgressSymbolBrush;
	
	// ── 색 (추적 여부에 따른 2가지 상태) ─────────────────────────────

	/** 추적 중이 아닐 때의 위/아래 색. 기본은 검정 70% 불투명. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Color")
	FLinearColor IdleColorTop = FLinearColor(0.f, 0.f, 0.f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Color")
	FLinearColor IdleColorBottom = FLinearColor(0.f, 0.f, 0.f, 0.7f);

	/** 추적 중일 때의 위/아래 색. 두 값을 다르게 두면 세로 그라디언트가 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Color")
	FLinearColor TrackedColorTop = FLinearColor(0.35f, 1.f, 0.45f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Color")
	FLinearColor TrackedColorBottom = FLinearColor(0.02f, 0.45f, 0.15f, 1.f);

	/** 심볼에 적용할 색 — 배경과 대비되도록 배경 색과 분리해 둔다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Color")
	FLinearColor SymbolColor = FLinearColor::White;

	// ── 머티리얼 파라미터 이름 (M_UI_QuestMarker 계약) ────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Material")
	FName ColorTopParam = TEXT("ColorTop");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest|Material")
	FName ColorBottomParam = TEXT("ColorBottom");

private:
	/** Img_Background 브러시에서 만든 동적 머티리얼 인스턴스 (없으면 단색 틴트 폴백). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BackgroundMID;
};
