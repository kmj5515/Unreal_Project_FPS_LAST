#pragma once

#include "CoreMinimal.h"
#include "Debug/GasInspector/LastFPSGasSnapshot.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class STextBlock;

// 인스펙터가 한 번에 보여줄 정보 범주(탭). 양 열(플레이어/대상)에 동일하게 적용된다.
enum class ELastFPSGasInspectorTab : uint8
{
	Attributes,
	Effects,
	Tags,
	Abilities
};

/**
 * 런타임 GAS 인스펙터의 화면 표시(뷰).
 *
 * 게임 뷰포트에 얹히는 오버레이다. GAS를 직접 읽지 않고 스냅샷만 받아 그린다.
 * 상단 탭/버튼만 클릭을 받고, 정보 표시 영역은 히트테스트에서 제외해
 * 월드 클릭(스포이드)이 항상 통과하도록 한다.
 */
class SLastFPSGasInspectorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLastFPSGasInspectorPanel) {}
		// "다른 캐릭터 지정" 버튼을 눌렀을 때 호출(컨트롤러가 픽을 무장한다).
		SLATE_EVENT(FSimpleDelegate, OnRequestPick)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 플레이어/대상 스냅샷을 캐시하고 현재 탭으로 두 열을 다시 그린다.
	void RefreshSnapshots(const FLastFPSGasSnapshot& PlayerSnapshot, const FLastFPSGasSnapshot& TargetSnapshot);

	// 상단 안내 문구 갱신(예: "대상을 클릭하세요").
	void SetHintText(const FText& InHint);

private:
	TSharedRef<SWidget> MakeTabButton(const FText& Label, ELastFPSGasInspectorTab Tab);
	void SetActiveTab(ELastFPSGasInspectorTab Tab);
	FReply HandlePickClicked();

	// 캐시된 스냅샷으로 두 열의 위젯 트리를 다시 만든다(구조가 바뀔 때만 호출).
	void RebuildColumns();
	void PopulateColumn(const TSharedRef<SVerticalBox>& ColumnBox, const FLastFPSGasSnapshot& Snapshot, bool bIsPlayerColumn) const;

	// 현재 탭에 해당하는 표시 라인들을 스냅샷에서 뽑는다.
	TArray<FText> BuildLinesForActiveTab(const FLastFPSGasSnapshot& Snapshot) const;
	FText GetActiveTabTitle() const;

	// 캐시된 스냅샷+활성 탭으로 표시 라인 캐시를 다시 계산한다.
	void RecomputeCachedLines();
	// 행 개수/대상/탭 등 "구조"를 나타내는 서명. 값만 바뀔 때는 재구성하지 않기 위한 비교용.
	FString ComputeStructureSignature() const;

	TSharedPtr<SVerticalBox> PlayerColumnBox;
	TSharedPtr<SVerticalBox> TargetColumnBox;
	TSharedPtr<STextBlock> HintTextBlock;

	FSimpleDelegate OnRequestPickDelegate;
	ELastFPSGasInspectorTab ActiveTab = ELastFPSGasInspectorTab::Attributes;

	// 탭 전환 시 즉시 다시 그릴 수 있도록 마지막 스냅샷을 보관한다.
	FLastFPSGasSnapshot CachedPlayerSnapshot;
	FLastFPSGasSnapshot CachedTargetSnapshot;

	// 활성 탭의 표시 라인 캐시. 텍스트 위젯이 이 배열을 람다로 읽어 매 프레임 최신값을 반영한다.
	TArray<FText> CachedPlayerLines;
	TArray<FText> CachedTargetLines;

	// 마지막으로 위젯을 재구성했을 때의 구조 서명. 값만 변하면 재구성을 건너뛴다.
	FString LastStructureSignature;
};
