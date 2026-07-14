#include "Debug/GasInspector/SLastFPSGasInspectorPanel.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSGasInspectorPanel"

namespace
{
	// 런타임 오버레이 전용 색/브러시. 에디터 스타일에 의존하지 않도록 이 파일 안에서 정의한다.
	FLinearColor PanelBgColor() { return FLinearColor(0.02f, 0.02f, 0.03f, 0.82f); }
	FLinearColor TitleColor() { return FLinearColor(0.86f, 0.88f, 0.92f, 1.f); }
	FLinearColor MutedColor() { return FLinearColor(0.58f, 0.60f, 0.66f, 1.f); }
	FLinearColor AccentColor() { return FLinearColor(0.20f, 0.62f, 0.95f, 1.f); }
	FLinearColor InactiveTabColor() { return FLinearColor(0.13f, 0.135f, 0.15f, 0.95f); }
	FLinearColor OnAccentColor() { return FLinearColor(0.02f, 0.04f, 0.07f, 1.f); }

	const FSlateBrush* GetSolidBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	// ButtonColorAndOpacity로 색을 입히기 위한 흰색 라운드 브러시 기반 버튼 스타일.
	const FButtonStyle& GetFlatButtonStyle()
	{
		static const FButtonStyle Style = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White, 4.f))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 4.f))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor::White, 4.f))
			.SetNormalPadding(FMargin(0.f))
			.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	FSlateFontInfo TitleFont() { return FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 17); }
	FSlateFontInfo HeadingFont() { return FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 15); }
	FSlateFontInfo BodyFont() { return FCoreStyle::GetDefaultFontStyle(FName(TEXT("Regular")), 13); }

	FText FormatValue(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 1;
		return FText::AsNumber(Value, &Options);
	}
}

void SLastFPSGasInspectorPanel::Construct(const FArguments& InArgs)
{
	OnRequestPickDelegate = InArgs._OnRequestPick;

	// 오버레이 루트는 자기 자신을 히트테스트에서 제외한다(자식 버튼만 클릭 수용).
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(GetSolidBrush())
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.f))
		.Visibility(EVisibility::SelfHitTestInvisible)
		.Padding(FMargin(24.f, 18.f))
		[
			SNew(SVerticalBox)

			// ── 상단 바: 탭 + 안내 + "다른 캐릭터 지정" ─────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 10.f)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)

				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[ MakeTabButton(LOCTEXT("TabAttributes", "어트리뷰트"), ELastFPSGasInspectorTab::Attributes) ]

				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[ MakeTabButton(LOCTEXT("TabEffects", "활성 이펙트"), ELastFPSGasInspectorTab::Effects) ]

				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[ MakeTabButton(LOCTEXT("TabTags", "보유 태그"), ELastFPSGasInspectorTab::Tags) ]

				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
				[ MakeTabButton(LOCTEXT("TabAbilities", "부여된 어빌리티"), ELastFPSGasInspectorTab::Abilities) ]

				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SAssignNew(HintTextBlock, STextBlock)
					.Text(LOCTEXT("DefaultHint", "캐릭터를 클릭해 스포이드로 선택하세요"))
					.ColorAndOpacity(MutedColor())
					.Font(BodyFont())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&GetFlatButtonStyle())
					.ContentPadding(FMargin(12.f, 5.f))
					.ButtonColorAndOpacity(AccentColor())
					.ToolTipText(LOCTEXT("PickTooltip", "누른 뒤 다른 캐릭터를 클릭하면 대상이 바뀝니다"))
					.OnClicked(this, &SLastFPSGasInspectorPanel::HandlePickClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PickButton", "다른 캐릭터 지정"))
						.ColorAndOpacity(OnAccentColor())
						.Font(HeadingFont())
					]
				]
			]

			// ── 본문: 플레이어(좌) / 대상(우) ───────────────────────────
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.WidthOverride(340.f)
					[
						SNew(SBorder)
						.BorderImage(GetSolidBrush())
						.BorderBackgroundColor(PanelBgColor())
						.Visibility(EVisibility::HitTestInvisible)
						.Padding(FMargin(12.f, 10.f))
						[
							SAssignNew(PlayerColumnBox, SVerticalBox)
						]
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNullWidget::NullWidget
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.WidthOverride(340.f)
					[
						SNew(SBorder)
						.BorderImage(GetSolidBrush())
						.BorderBackgroundColor(PanelBgColor())
						.Visibility(EVisibility::HitTestInvisible)
						.Padding(FMargin(12.f, 10.f))
						[
							SAssignNew(TargetColumnBox, SVerticalBox)
						]
					]
				]
			]
		]
	];

	RebuildColumns();
}

TSharedRef<SWidget> SLastFPSGasInspectorPanel::MakeTabButton(const FText& Label, ELastFPSGasInspectorTab Tab)
{
	return SNew(SButton)
		.ButtonStyle(&GetFlatButtonStyle())
		.ContentPadding(FMargin(11.f, 5.f))
		// 활성 탭은 강조색, 비활성 탭은 어두운색으로 자동 갱신된다.
		.ButtonColorAndOpacity_Lambda([this, Tab]()
		{
			return FSlateColor(ActiveTab == Tab ? AccentColor() : InactiveTabColor());
		})
		.OnClicked_Lambda([this, Tab]()
		{
			SetActiveTab(Tab);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(HeadingFont())
			.ColorAndOpacity_Lambda([this, Tab]()
			{
				return FSlateColor(ActiveTab == Tab ? OnAccentColor() : TitleColor());
			})
		];
}

void SLastFPSGasInspectorPanel::SetActiveTab(ELastFPSGasInspectorTab Tab)
{
	if (ActiveTab == Tab)
	{
		return;
	}

	ActiveTab = Tab;
	RecomputeCachedLines();
	LastStructureSignature = ComputeStructureSignature();
	RebuildColumns();
}

FReply SLastFPSGasInspectorPanel::HandlePickClicked()
{
	OnRequestPickDelegate.ExecuteIfBound();
	return FReply::Handled();
}

void SLastFPSGasInspectorPanel::SetHintText(const FText& InHint)
{
	if (HintTextBlock.IsValid())
	{
		HintTextBlock->SetText(InHint);
	}
}

void SLastFPSGasInspectorPanel::RefreshSnapshots(const FLastFPSGasSnapshot& PlayerSnapshot, const FLastFPSGasSnapshot& TargetSnapshot)
{
	CachedPlayerSnapshot = PlayerSnapshot;
	CachedTargetSnapshot = TargetSnapshot;
	RecomputeCachedLines();

	// 행 개수/대상/탭 같은 구조가 바뀔 때만 위젯을 다시 만든다.
	// 값만 바뀐 경우엔 기존 텍스트 위젯이 람다로 캐시를 읽어 깜박임 없이 갱신된다.
	const FString Signature = ComputeStructureSignature();
	if (Signature != LastStructureSignature)
	{
		LastStructureSignature = Signature;
		RebuildColumns();
	}
}

void SLastFPSGasInspectorPanel::RecomputeCachedLines()
{
	CachedPlayerLines = BuildLinesForActiveTab(CachedPlayerSnapshot);
	CachedTargetLines = BuildLinesForActiveTab(CachedTargetSnapshot);
}

FString SLastFPSGasInspectorPanel::ComputeStructureSignature() const
{
	auto ColumnSignature = [](const FLastFPSGasSnapshot& Snapshot, int32 LineCount)
	{
		return FString::Printf(TEXT("%d~%s~%s~%d"),
			Snapshot.bValid ? 1 : 0,
			*Snapshot.DisplayName,
			*Snapshot.OwnerClassName,
			LineCount);
	};

	return FString::Printf(TEXT("%d|%s|%s"),
		static_cast<int32>(ActiveTab),
		*ColumnSignature(CachedPlayerSnapshot, CachedPlayerLines.Num()),
		*ColumnSignature(CachedTargetSnapshot, CachedTargetLines.Num()));
}

void SLastFPSGasInspectorPanel::RebuildColumns()
{
	if (PlayerColumnBox.IsValid())
	{
		PopulateColumn(PlayerColumnBox.ToSharedRef(), CachedPlayerSnapshot, /*bIsPlayerColumn=*/true);
	}

	if (TargetColumnBox.IsValid())
	{
		PopulateColumn(TargetColumnBox.ToSharedRef(), CachedTargetSnapshot, /*bIsPlayerColumn=*/false);
	}
}

void SLastFPSGasInspectorPanel::PopulateColumn(const TSharedRef<SVerticalBox>& ColumnBox, const FLastFPSGasSnapshot& Snapshot, bool bIsPlayerColumn) const
{
	ColumnBox->ClearChildren();

	// 헤더: 표시명 + 클래스명.
	ColumnBox->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(Snapshot.DisplayName.IsEmpty()
			? LOCTEXT("NoTarget", "대상 없음")
			: FText::FromString(Snapshot.DisplayName))
		.ColorAndOpacity(TitleColor())
		.Font(TitleFont())
	];

	if (!Snapshot.OwnerClassName.IsEmpty())
	{
		ColumnBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 1.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Snapshot.OwnerClassName))
			.ColorAndOpacity(MutedColor())
			.Font(BodyFont())
		];
	}

	if (!Snapshot.bValid)
	{
		ColumnBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoAsc", "AbilitySystemComponent 없음"))
			.ColorAndOpacity(MutedColor())
			.Font(BodyFont())
		];
		return;
	}

	// 이 열의 라인 캐시. 값 변화는 위젯을 파괴하지 않고 이 배열을 통해 반영된다.
	const int32 LineCount = bIsPlayerColumn ? CachedPlayerLines.Num() : CachedTargetLines.Num();

	// 활성 탭 제목 + 개수(개수 변화는 구조 서명에 반영되어 재구성된다).
	ColumnBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 10.f, 0.f, 3.f)
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("SectionTitleFmt", "{0}  ({1})"), GetActiveTabTitle(), FText::AsNumber(LineCount)))
		.ColorAndOpacity(AccentColor())
		.Font(HeadingFont())
	];

	if (LineCount == 0)
	{
		ColumnBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SectionEmpty", "(없음)"))
			.ColorAndOpacity(MutedColor())
			.Font(BodyFont())
		];
		return;
	}

	// 각 라인 텍스트는 캐시 배열을 람다로 읽는다 → 값만 바뀌면 위젯 재생성 없이 갱신(깜박임 방지).
	for (int32 LineIndex = 0; LineIndex < LineCount; ++LineIndex)
	{
		ColumnBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text_Lambda([this, bIsPlayerColumn, LineIndex]()
			{
				const TArray<FText>& Lines = bIsPlayerColumn ? CachedPlayerLines : CachedTargetLines;
				return Lines.IsValidIndex(LineIndex) ? Lines[LineIndex] : FText::GetEmpty();
			})
			.AutoWrapText(true)
			.ColorAndOpacity(MutedColor())
			.Font(BodyFont())
		];
	}
}

TArray<FText> SLastFPSGasInspectorPanel::BuildLinesForActiveTab(const FLastFPSGasSnapshot& Snapshot) const
{
	TArray<FText> Lines;

	switch (ActiveTab)
	{
	case ELastFPSGasInspectorTab::Attributes:
		Lines.Reserve(Snapshot.Attributes.Num());
		for (const FLastFPSGasAttributeSnapshot& Attribute : Snapshot.Attributes)
		{
			Lines.Add(FText::Format(
				LOCTEXT("AttrLine", "{0} :  {1}   (base {2})"),
				FText::FromString(Attribute.Name),
				FormatValue(Attribute.CurrentValue),
				FormatValue(Attribute.BaseValue)));
		}
		break;

	case ELastFPSGasInspectorTab::Effects:
		Lines.Reserve(Snapshot.Effects.Num());
		for (const FLastFPSGasEffectSnapshot& Effect : Snapshot.Effects)
		{
			const FText DurationText = Effect.Duration > 0.f
				? FText::Format(LOCTEXT("EffectDuration", "{0}/{1}s"), FormatValue(Effect.TimeRemaining), FormatValue(Effect.Duration))
				: LOCTEXT("EffectInfinite", "지속");
			Lines.Add(FText::Format(
				LOCTEXT("EffectLine", "{0}   {1}   x{2}"),
				FText::FromString(Effect.Name),
				DurationText,
				FText::AsNumber(Effect.StackCount)));
		}
		break;

	case ELastFPSGasInspectorTab::Tags:
		Lines.Reserve(Snapshot.OwnedTags.Num());
		for (const FString& tag : Snapshot.OwnedTags)
		{
			Lines.Add(FText::FromString(tag));
		}
		break;

	case ELastFPSGasInspectorTab::Abilities:
		Lines.Reserve(Snapshot.Abilities.Num());
		for (const FLastFPSGasAbilitySnapshot& Ability : Snapshot.Abilities)
		{
			Lines.Add(Ability.bActive
				? FText::Format(LOCTEXT("AbilityActive", "{0}   [활성]"), FText::FromString(Ability.Name))
				: FText::FromString(Ability.Name));
		}
		break;
	}

	return Lines;
}

FText SLastFPSGasInspectorPanel::GetActiveTabTitle() const
{
	switch (ActiveTab)
	{
	case ELastFPSGasInspectorTab::Attributes: return LOCTEXT("TabAttributes", "어트리뷰트");
	case ELastFPSGasInspectorTab::Effects:    return LOCTEXT("TabEffects", "활성 이펙트");
	case ELastFPSGasInspectorTab::Tags:       return LOCTEXT("TabTags", "보유 태그");
	case ELastFPSGasInspectorTab::Abilities:  return LOCTEXT("TabAbilities", "부여된 어빌리티");
	default:                                   return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
