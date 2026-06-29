#include "SLastFPSRuntimeStatsEditor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Editor.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSRuntimeStatsEditor"

void SLastFPSRuntimeStatsEditor::Construct(const FArguments& InArgs)
{
	RefreshFromTarget();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "Runtime Character Stats"))
				.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 14))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Refresh", "Refresh"))
					.OnClicked(this, &SLastFPSRuntimeStatsEditor::RefreshClicked)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Apply", "Apply"))
					.OnClicked(this, &SLastFPSRuntimeStatsEditor::ApplyToTarget)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ForceCritical", "Crit Test 100%"))
					.OnClicked(this, &SLastFPSRuntimeStatsEditor::ForceCriticalTest)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("Health", "Health"), &Health, 0.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("MaxHealth", "Max Health"), &MaxHealth, 1.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("Stamina", "Stamina"), &Stamina, 0.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("MaxStamina", "Max Stamina"), &MaxStamina, 1.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("AttackDamage", "Attack Damage"), &AttackDamage, 0.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("CriticalChance", "Critical Chance (%)"), &CriticalChance, 0.f, 100.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("CriticalDamage", "Critical Damage (%)"), &CriticalDamagePercent, 100.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("Defense", "Defense"), &Defense, 0.f)]
				+ SScrollBox::Slot()[BuildStatRow(LOCTEXT("MoveSpeed", "Move Speed"), &MoveSpeed, 0.f)]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(LOCTEXT("Ready", "Open PIE and press Refresh."))
			]
		]
	];

	RefreshFromTarget();
}

TSharedRef<SWidget> SLastFPSRuntimeStatsEditor::BuildStatRow(
	const FText& Label,
	float* ValuePtr,
	float MinValue,
	float MaxValue)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.45f)
		.VAlign(VAlign_Center)
		.Padding(0.f, 3.f, 8.f, 3.f)
		[
			SNew(STextBlock)
			.Text(Label)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.55f)
		.Padding(0.f, 3.f)
		[
			SNew(SSpinBox<float>)
			.MinValue(MinValue)
			.MaxValue(MaxValue)
			.MinSliderValue(MinValue)
			.MaxSliderValue(MaxValue == TNumericLimits<float>::Max() ? TOptional<float>() : TOptional<float>(MaxValue))
			.Value_Lambda([ValuePtr]() { return ValuePtr ? *ValuePtr : 0.f; })
			.OnValueChanged_Lambda([ValuePtr](float NewValue)
			{
				if (ValuePtr)
				{
					*ValuePtr = NewValue;
				}
			})
		];
}

UAbilitySystemComponent* SLastFPSRuntimeStatsEditor::FindTargetAbilitySystem() const
{
	UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
	if (!PlayWorld)
	{
		return nullptr;
	}

	APlayerController* PlayerController = PlayWorld->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Pawn);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}

void SLastFPSRuntimeStatsEditor::RefreshFromTarget()
{
	UAbilitySystemComponent* ASC = FindTargetAbilitySystem();
	const ULastFPSAttributeSet* Attributes = ASC ? ASC->GetSet<ULastFPSAttributeSet>() : nullptr;
	if (!Attributes)
	{
		SetStatus(LOCTEXT("NoTarget", "No PIE player AbilitySystem found."));
		return;
	}

	Health = Attributes->GetHealth();
	MaxHealth = Attributes->GetMaxHealth();
	Stamina = Attributes->GetStamina();
	MaxStamina = Attributes->GetMaxStamina();
	AttackDamage = Attributes->GetAttackDamage();
	CriticalChance = Attributes->GetCriticalChance();
	CriticalDamagePercent = Attributes->GetCriticalDamagePercent();
	Defense = Attributes->GetDefense();
	MoveSpeed = Attributes->GetMoveSpeed();
	SetStatus(LOCTEXT("Refreshed", "Loaded current PIE character stats."));
}

FReply SLastFPSRuntimeStatsEditor::ApplyToTarget()
{
	UAbilitySystemComponent* ASC = FindTargetAbilitySystem();
	if (!ASC)
	{
		SetStatus(LOCTEXT("ApplyNoTarget", "No PIE player AbilitySystem found."));
		return FReply::Handled();
	}

	const float ClampedMaxHealth = FMath::Max(MaxHealth, 1.f);
	const float ClampedMaxStamina = FMath::Max(MaxStamina, 1.f);

	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMaxHealthAttribute(), ClampedMaxHealth);
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetHealthAttribute(), FMath::Clamp(Health, 0.f, ClampedMaxHealth));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMaxStaminaAttribute(), ClampedMaxStamina);
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetStaminaAttribute(), FMath::Clamp(Stamina, 0.f, ClampedMaxStamina));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetAttackDamageAttribute(), FMath::Max(AttackDamage, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetCriticalChanceAttribute(), FMath::Clamp(CriticalChance, 0.f, 100.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetCriticalDamagePercentAttribute(), FMath::Max(CriticalDamagePercent, 100.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetDefenseAttribute(), FMath::Max(Defense, 0.f));
	ASC->SetNumericAttributeBase(ULastFPSAttributeSet::GetMoveSpeedAttribute(), FMath::Max(MoveSpeed, 0.f));

	SetStatus(LOCTEXT("Applied", "Applied stats to current PIE character."));
	return FReply::Handled();
}

FReply SLastFPSRuntimeStatsEditor::ForceCriticalTest()
{
	CriticalChance = 100.f;
	CriticalDamagePercent = 200.f;
	return ApplyToTarget();
}

FReply SLastFPSRuntimeStatsEditor::RefreshClicked()
{
	RefreshFromTarget();
	return FReply::Handled();
}

void SLastFPSRuntimeStatsEditor::SetStatus(const FText& NewStatus)
{
	if (StatusText)
	{
		StatusText->SetText(NewStatus);
	}
}

#undef LOCTEXT_NAMESPACE
