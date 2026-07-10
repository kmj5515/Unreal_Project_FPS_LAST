#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

struct FButtonStyle;
struct FEditableTextBoxStyle;
class SWidget;

/**
 * 에디터 유틸리티 툴들에서 공통으로 재사용하는 Slate UI 헬퍼 모음.
 * 특정 툴(SCharacterDataAssetTool 등)의 상태에 의존하지 않는 순수 함수라
 * 어떤 SCompoundWidget/툴에서도 그대로 호출할 수 있다.
 */
namespace LastFPSEditorWidgets
{
	EDITORUTILITY_API FLinearColor GetToolBackplateColor();
	EDITORUTILITY_API FLinearColor GetToolHeaderColor();
	EDITORUTILITY_API FLinearColor GetToolSectionColor();
	EDITORUTILITY_API FLinearColor GetToolRowColor();
	EDITORUTILITY_API FLinearColor GetToolAccentColor();
	EDITORUTILITY_API FLinearColor GetToolTextColor();
	EDITORUTILITY_API FLinearColor GetToolMutedTextColor();
	EDITORUTILITY_API const FButtonStyle& GetToolButtonStyle();
	EDITORUTILITY_API const FEditableTextBoxStyle& GetToolEditableTextBoxStyle();

	EDITORUTILITY_API TSharedRef<SWidget> MakeToolPanel(
		const FText& Title,
		const FText& Subtitle,
		const TSharedRef<SWidget>& BodyContent);

	EDITORUTILITY_API TSharedRef<SWidget> MakeFormRow(
		const FText& LabelText,
		const TSharedRef<SWidget>& ValueContent,
		float LabelWidth = 210.f);

	EDITORUTILITY_API TSharedRef<SWidget> MakeRowBox(
		const TSharedRef<SWidget>& BodyContent);

	/**
	 * 컬러 구분선 위젯.
	 * @param LineColor 선 색
	 * @param Size      선 크기(가로는 부모 폭에 맞춰 늘어나고 세로가 두께)
	 * @param Padding   선 주변 여백
	 */
	EDITORUTILITY_API TSharedRef<SWidget> MakeColorLine(
		const FLinearColor& LineColor,
		const FVector2D& Size = FVector2D(2.f, 2.f),
		const FMargin& Padding = FMargin(0.f));

	/**
	 * Odin FoldoutGroup 풍의 접이식 섹션.
	 * 그룹 보더(카드) + 헤더(이름 + 컬러 라인) + 들여쓰기된 접이식 본문으로 구성된다.
	 * @param Title              섹션 이름(헤더에 표시)
	 * @param LineColor          이름 아래 구분선 색
	 * @param BodyContent        접었다 펼 본문 위젯
	 * @param bInitiallyExpanded 처음에 펼쳐진 상태로 둘지 여부
	 */
	EDITORUTILITY_API TSharedRef<SWidget> MakeSection(
		const FText& Title,
		const FLinearColor& LineColor,
		const TSharedRef<SWidget>& BodyContent,
		bool bInitiallyExpanded = true);
}
