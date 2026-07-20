#include "LastFPSEditorWidgets.h"

#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FLinearColor ToolBackplateColor()
	{
		return FLinearColor(0.015f, 0.016f, 0.018f, 0.96f);
	}

	FLinearColor ToolHeaderColor()
	{
		return FLinearColor(0.035f, 0.036f, 0.04f, 1.f);
	}

	FLinearColor ToolSectionColor()
	{
		return FLinearColor(0.07f, 0.07f, 0.075f, 0.94f);
	}

	FLinearColor ToolRowColor()
	{
		return FLinearColor(0.12f, 0.12f, 0.125f, 0.86f);
	}

	FLinearColor ToolAccentColor()
	{
		return FLinearColor(0.14f, 0.58f, 0.92f, 1.f);
	}

	FLinearColor ToolTextColor()
	{
		return FLinearColor(0.82f, 0.84f, 0.86f, 1.f);
	}

	FLinearColor ToolMutedTextColor()
	{
		return FLinearColor(0.46f, 0.48f, 0.52f, 1.f);
	}

	FString GetToolPanelImagePath(const TCHAR* FileName)
	{
		static const FString ResourceDirectory = []()
		{
			if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EditorUtility")))
			{
				return Plugin->GetBaseDir() / TEXT("Resources/ToolPanel");
			}

			return FPaths::ProjectDir() / TEXT("Plugins/EditorUtility/Resources/ToolPanel");
		}();

		return ResourceDirectory / FileName;
	}

	const FSlateBrush* GetBackplateFallbackBrush()
	{
		static const FSlateRoundedBoxBrush Brush(ToolBackplateColor(), 10.f);
		return &Brush;
	}

	const FSlateBrush* GetBackplateBrush()
	{
		static const FSlateBoxBrush Brush(GetToolPanelImagePath(TEXT("TP_Backplate.png")), FMargin(0.28f));
		return &Brush;
	}

	const FSlateBrush* GetHeaderFallbackBrush()
	{
		static const FSlateRoundedBoxBrush Brush(ToolHeaderColor(), 8.f);
		return &Brush;
	}

	const FSlateBrush* GetHeaderBrush()
	{
		static const FSlateBoxBrush Brush(GetToolPanelImagePath(TEXT("TP_Header.png")), FMargin(0.26f));
		return &Brush;
	}

	const FSlateBrush* GetSectionFallbackBrush()
	{
		static const FSlateRoundedBoxBrush Brush(ToolSectionColor(), 8.f);
		return &Brush;
	}

	const FSlateBrush* GetSectionBrush()
	{
		static const FSlateBoxBrush Brush(GetToolPanelImagePath(TEXT("TP_Section.png")), FMargin(0.24f));
		return &Brush;
	}

	const FSlateBrush* GetRowFallbackBrush()
	{
		static const FSlateRoundedBoxBrush Brush(ToolRowColor(), 6.f);
		return &Brush;
	}

	const FSlateBrush* GetRowBrush()
	{
		static const FSlateBoxBrush Brush(GetToolPanelImagePath(TEXT("TP_Row.png")), FMargin(0.24f));
		return &Brush;
	}

	const FSlateBrush& GetFieldBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.055f, 0.056f, 0.06f, 1.f), 5.f);
		return Brush;
	}

	const FSlateBrush& GetButtonBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.09f, 0.093f, 0.10f, 1.f), 5.f);
		return Brush;
	}

	const FSlateBrush& GetButtonPressedBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.08f, 0.26f, 0.42f, 1.f), 5.f);
		return Brush;
	}

	const FSlateBrush* GetAccentLineBrush()
	{
		static const FSlateRoundedBoxBrush Brush(ToolAccentColor(), 1.f);
		return &Brush;
	}

	TSharedRef<SWidget> MakeImageBackedBorder(
		const FSlateBrush* FallbackBrush,
		const FSlateBrush*,
		const FMargin& Padding,
		const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder)
			.BorderImage(FallbackBrush)
			.Padding(Padding)
			[
				Content
			];
	}
}

namespace LastFPSEditorWidgets
{
	FLinearColor GetToolBackplateColor()
	{
		return ToolBackplateColor();
	}

	FLinearColor GetToolHeaderColor()
	{
		return ToolHeaderColor();
	}

	FLinearColor GetToolSectionColor()
	{
		return ToolSectionColor();
	}

	FLinearColor GetToolRowColor()
	{
		return ToolRowColor();
	}

	FLinearColor GetToolAccentColor()
	{
		return ToolAccentColor();
	}

	FLinearColor GetToolTextColor()
	{
		return ToolTextColor();
	}

	FLinearColor GetToolMutedTextColor()
	{
		return ToolMutedTextColor();
	}

	const FButtonStyle& GetToolButtonStyle()
	{
		static const FButtonStyle Style = FButtonStyle()
			.SetNormal(GetButtonBrush())
			.SetHovered(GetButtonBrush())
			.SetPressed(GetButtonPressedBrush())
			.SetDisabled(GetFieldBrush())
			.SetNormalPadding(FMargin(8.f, 2.f))
			.SetPressedPadding(FMargin(8.f, 3.f, 8.f, 1.f));

		return Style;
	}

	const FEditableTextBoxStyle& GetToolEditableTextBoxStyle()
	{
		static const FEditableTextBoxStyle Style = FEditableTextBoxStyle(FEditableTextBoxStyle::GetDefault())
			.SetBackgroundImageNormal(GetFieldBrush())
			.SetBackgroundImageHovered(GetFieldBrush())
			.SetBackgroundImageFocused(GetFieldBrush())
			.SetBackgroundImageReadOnly(GetFieldBrush())
			.SetForegroundColor(FSlateColor(GetToolTextColor()))
			.SetFocusedForegroundColor(FSlateColor(GetToolTextColor()))
			.SetReadOnlyForegroundColor(FSlateColor(GetToolMutedTextColor()))
			.SetPadding(FMargin(8.f, 3.f));

		return Style;
	}

	TSharedRef<SWidget> MakeToolPanel(
		const FText& Title,
		const FText& Subtitle,
		const TSharedRef<SWidget>& BodyContent)
	{
		return MakeImageBackedBorder(
			GetBackplateFallbackBrush(),
			GetBackplateBrush(),
			8.f,
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeImageBackedBorder(
					GetHeaderFallbackBrush(),
					GetHeaderBrush(),
					FMargin(10.f, 7.f),
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("[]")))
						.ColorAndOpacity(GetToolMutedTextColor())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(Title)
						.ColorAndOpacity(GetToolTextColor())
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(6.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(Subtitle)
						.ColorAndOpacity(GetToolMutedTextColor())
					])
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 5.f, 0.f, 0.f)
			[
				BodyContent
			]);
	}

	TSharedRef<SWidget> MakeFormRow(
		const FText& LabelText,
		const TSharedRef<SWidget>& ValueContent,
		float LabelWidth)
	{
		return MakeRowBox(
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(LabelWidth)
				[
					SNew(STextBlock)
					.Text(LabelText)
					.ColorAndOpacity(GetToolTextColor())
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				ValueContent
			]);
	}

	TSharedRef<SWidget> MakeRowBox(const TSharedRef<SWidget>& BodyContent)
	{
		return MakeImageBackedBorder(
			GetRowFallbackBrush(),
			GetRowBrush(),
			FMargin(10.f, 6.f),
			BodyContent);
	}

	TSharedRef<SWidget> MakeColorLine(
		const FLinearColor& LineColor,
		const FVector2D& Size,
		const FMargin& Padding)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Padding)
			[
				SNew(SBox)
				.HeightOverride(Size.Y)
				[
					SNew(SBorder)
					.BorderImage(GetAccentLineBrush())
					.BorderBackgroundColor(LineColor)
				]
			];
	}

	TSharedRef<SWidget> MakeSection(
		const FText& Title,
		const FLinearColor& LineColor,
		const TSharedRef<SWidget>& BodyContent,
		bool bInitiallyExpanded)
	{
		return MakeImageBackedBorder(
			GetSectionFallbackBrush(),
			GetSectionBrush(),
			FMargin(2.f),
			SNew(SExpandableArea)
			.InitiallyCollapsed(!bInitiallyExpanded)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			.BodyBorderImage(FAppStyle::GetBrush("NoBorder"))
			.HeaderPadding(FMargin(8.f, 7.f))
			.Padding(FMargin(8.f, 6.f, 8.f, 8.f))
			.HeaderContent()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Text(Title)
					.ColorAndOpacity(GetToolTextColor())
					.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeColorLine(LineColor)
				]
			]
			.BodyContent()
			[
				BodyContent
			]);
	}
}
