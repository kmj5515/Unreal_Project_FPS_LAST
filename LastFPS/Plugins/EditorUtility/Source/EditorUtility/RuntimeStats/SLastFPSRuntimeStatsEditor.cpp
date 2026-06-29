#include "RuntimeStats/SLastFPSRuntimeStatsEditor.h"

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
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLastFPSRuntimeStatsEditor"

void SLastFPSRuntimeStatsEditor::Construct(const FArguments& InArgs)
{
	InitializeStatRows();
	RefreshFromTarget();

	TSharedRef<SScrollBox> StatsScrollBox = SNew(SScrollBox);
	StatsScrollBox->AddSlot()[BuildHeaderRow()];
	for (int32 RowIndex = 0; RowIndex < StatRows.Num(); ++RowIndex)
	{
		StatsScrollBox->AddSlot()[BuildStatRow(RowIndex)];
	}

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
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetDelta", "Reset Delta"))
					.OnClicked(this, &SLastFPSRuntimeStatsEditor::ResetDeltasClicked)
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
				StatsScrollBox
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

void SLastFPSRuntimeStatsEditor::InitializeStatRows()
{
	StatRows.Reset();
	StatRows.Add({ LOCTEXT("Health", "Health"), ULastFPSAttributeSet::GetHealthAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 100.f });
	StatRows.Add({ LOCTEXT("MaxHealth", "Max Health"), ULastFPSAttributeSet::GetMaxHealthAttribute(), 0.f, 0.f, 1.f, TNumericLimits<float>::Max(), 100.f });
	StatRows.Add({ LOCTEXT("Stamina", "Stamina"), ULastFPSAttributeSet::GetStaminaAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 10.f });
	StatRows.Add({ LOCTEXT("MaxStamina", "Max Stamina"), ULastFPSAttributeSet::GetMaxStaminaAttribute(), 0.f, 0.f, 1.f, TNumericLimits<float>::Max(), 10.f });
	StatRows.Add({ LOCTEXT("AttackDamage", "Attack Damage"), ULastFPSAttributeSet::GetAttackDamageAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 10.f });
	StatRows.Add({ LOCTEXT("CriticalChance", "Critical Chance (%)"), ULastFPSAttributeSet::GetCriticalChanceAttribute(), 0.f, 0.f, 0.f, 100.f, 5.f });
	StatRows.Add({ LOCTEXT("CriticalDamage", "Critical Damage (%)"), ULastFPSAttributeSet::GetCriticalDamagePercentAttribute(), 0.f, 0.f, 100.f, TNumericLimits<float>::Max(), 50.f });
	StatRows.Add({ LOCTEXT("Defense", "Defense"), ULastFPSAttributeSet::GetDefenseAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 10.f });
	StatRows.Add({ LOCTEXT("PhysicalDamageMultiplier", "Physical Damage Multiplier"), ULastFPSAttributeSet::GetPhysicalDamageMultiplierAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 0.1f });
	StatRows.Add({ LOCTEXT("FireDamageMultiplier", "Fire Damage Multiplier"), ULastFPSAttributeSet::GetFireDamageMultiplierAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 0.1f });
	StatRows.Add({ LOCTEXT("IceDamageMultiplier", "Ice Damage Multiplier"), ULastFPSAttributeSet::GetIceDamageMultiplierAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 0.1f });
	StatRows.Add({ LOCTEXT("ElectricDamageMultiplier", "Electric Damage Multiplier"), ULastFPSAttributeSet::GetElectricDamageMultiplierAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 0.1f });
	StatRows.Add({ LOCTEXT("PoisonDamageMultiplier", "Poison Damage Multiplier"), ULastFPSAttributeSet::GetPoisonDamageMultiplierAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 0.1f });
	StatRows.Add({ LOCTEXT("MoveSpeed", "Move Speed"), ULastFPSAttributeSet::GetMoveSpeedAttribute(), 0.f, 0.f, 0.f, TNumericLimits<float>::Max(), 100.f });
}

TSharedRef<SWidget> SLastFPSRuntimeStatsEditor::BuildHeaderRow()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.28f)
		.Padding(0.f, 0.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StatHeader", "Stat"))
			.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.20f)
		.Padding(0.f, 0.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("InitialHeader", "처음 값"))
			.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.34f)
		.Padding(0.f, 0.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DeltaHeader", "변화량"))
			.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.18f)
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ResultHeader", "결과"))
			.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 10))
		];
}

TSharedRef<SWidget> SLastFPSRuntimeStatsEditor::BuildStatRow(int32 RowIndex)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.28f)
		.VAlign(VAlign_Center)
		.Padding(0.f, 3.f, 8.f, 3.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this, RowIndex]()
			{
				return StatRows.IsValidIndex(RowIndex) ? StatRows[RowIndex].Label : FText::GetEmpty();
			})
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.20f)
		.VAlign(VAlign_Center)
		.Padding(0.f, 3.f, 8.f, 3.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this, RowIndex]()
			{
				return StatRows.IsValidIndex(RowIndex) ? FText::AsNumber(StatRows[RowIndex].InitialValue) : FText::GetEmpty();
			})
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.34f)
		.Padding(0.f, 3.f, 8.f, 3.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("DecreaseDelta", "-"))
				.OnClicked(this, &SLastFPSRuntimeStatsEditor::AdjustDelta, RowIndex, -1.f)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpinBox<float>)
				.MinValue_Lambda([this, RowIndex]() -> TOptional<float>
				{
					if (!StatRows.IsValidIndex(RowIndex))
					{
						return TOptional<float>();
					}

					const FRuntimeStatRow& Row = StatRows[RowIndex];
					return Row.MinValue - Row.InitialValue;
				})
				.MaxValue_Lambda([this, RowIndex]() -> TOptional<float>
				{
					if (!StatRows.IsValidIndex(RowIndex))
					{
						return TOptional<float>();
					}

					const FRuntimeStatRow& Row = StatRows[RowIndex];
					const float MaxValue = GetRowMaxValue(Row);
					return MaxValue == TNumericLimits<float>::Max() ? TOptional<float>() : TOptional<float>(MaxValue - Row.InitialValue);
				})
				.MinSliderValue_Lambda([this, RowIndex]() -> TOptional<float>
				{
					if (!StatRows.IsValidIndex(RowIndex))
					{
						return TOptional<float>();
					}

					const FRuntimeStatRow& Row = StatRows[RowIndex];
					return Row.MinValue - Row.InitialValue;
				})
				.MaxSliderValue_Lambda([this, RowIndex]() -> TOptional<float>
				{
					if (!StatRows.IsValidIndex(RowIndex))
					{
						return TOptional<float>();
					}

					const FRuntimeStatRow& Row = StatRows[RowIndex];
					const float MaxValue = GetRowMaxValue(Row);
					return MaxValue == TNumericLimits<float>::Max() ? TOptional<float>() : TOptional<float>(MaxValue - Row.InitialValue);
				})
				.Value_Lambda([this, RowIndex]()
				{
					return StatRows.IsValidIndex(RowIndex) ? StatRows[RowIndex].DeltaValue : 0.f;
				})
				.OnValueChanged_Lambda([this, RowIndex](float NewValue)
				{
					if (StatRows.IsValidIndex(RowIndex))
					{
						FRuntimeStatRow& Row = StatRows[RowIndex];
						Row.DeltaValue = ClampDeltaForRow(Row, NewValue);
					}
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("IncreaseDelta", "+"))
				.OnClicked(this, &SLastFPSRuntimeStatsEditor::AdjustDelta, RowIndex, 1.f)
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.18f)
		.VAlign(VAlign_Center)
		.Padding(0.f, 3.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this, RowIndex]()
			{
				return StatRows.IsValidIndex(RowIndex) ? FText::AsNumber(GetRowResult(RowIndex)) : FText::GetEmpty();
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
	if (!ASC)
	{
		SetStatus(LOCTEXT("NoTarget", "No PIE player AbilitySystem found."));
		return;
	}

	for (FRuntimeStatRow& Row : StatRows)
	{
		Row.InitialValue = ASC->GetNumericAttribute(Row.Attribute);
		Row.DeltaValue = 0.f;
	}

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

	const int32 MaxHealthIndex = FindRowIndex(ULastFPSAttributeSet::GetMaxHealthAttribute());
	const int32 MaxStaminaIndex = FindRowIndex(ULastFPSAttributeSet::GetMaxStaminaAttribute());
	ApplyRowToTarget(*ASC, MaxHealthIndex);
	ApplyRowToTarget(*ASC, MaxStaminaIndex);

	for (int32 RowIndex = 0; RowIndex < StatRows.Num(); ++RowIndex)
	{
		if (RowIndex == MaxHealthIndex || RowIndex == MaxStaminaIndex)
		{
			continue;
		}

		ApplyRowToTarget(*ASC, RowIndex);
	}

	const AActor* OwnerActor = ASC->GetOwnerActor();
	const AActor* AvatarActor = ASC->GetAvatarActor();
	const UWorld* ASCWorld = ASC->GetWorld();

	UE_LOG(LogTemp, Warning,
		TEXT("[런타임스탯] 적용 대상 ASC=%s World=%s NetMode=%d Owner=%s Avatar=%s Authority=%d Role=%d Health=%f HealthBase=%f MaxHealth=%f MaxHealthBase=%f Stamina=%f StaminaBase=%f MaxStamina=%f MaxStaminaBase=%f MoveSpeed=%f MoveSpeedBase=%f"),
		*GetNameSafe(ASC),
		*GetNameSafe(ASCWorld),
		ASCWorld ? static_cast<int32>(ASCWorld->GetNetMode()) : INDEX_NONE,
		*GetNameSafe(OwnerActor),
		*GetNameSafe(AvatarActor),
		OwnerActor ? OwnerActor->HasAuthority() : false,
		OwnerActor ? static_cast<int32>(OwnerActor->GetLocalRole()) : INDEX_NONE,
		ASC->GetNumericAttribute(ULastFPSAttributeSet::GetHealthAttribute()),
		ASC->GetNumericAttributeBase(ULastFPSAttributeSet::GetHealthAttribute()),
		ASC->GetNumericAttribute(ULastFPSAttributeSet::GetMaxHealthAttribute()),
		ASC->GetNumericAttributeBase(ULastFPSAttributeSet::GetMaxHealthAttribute()),
		ASC->GetNumericAttribute(ULastFPSAttributeSet::GetStaminaAttribute()),
		ASC->GetNumericAttributeBase(ULastFPSAttributeSet::GetStaminaAttribute()),
		ASC->GetNumericAttribute(ULastFPSAttributeSet::GetMaxStaminaAttribute()),
		ASC->GetNumericAttributeBase(ULastFPSAttributeSet::GetMaxStaminaAttribute()),
		ASC->GetNumericAttribute(ULastFPSAttributeSet::GetMoveSpeedAttribute()),
		ASC->GetNumericAttributeBase(ULastFPSAttributeSet::GetMoveSpeedAttribute()));

	SetStatus(LOCTEXT("Applied", "Applied stats to current PIE character."));
	return FReply::Handled();
}

FReply SLastFPSRuntimeStatsEditor::ForceCriticalTest()
{
	const int32 CriticalChanceIndex = FindRowIndex(ULastFPSAttributeSet::GetCriticalChanceAttribute());
	if (StatRows.IsValidIndex(CriticalChanceIndex))
	{
		FRuntimeStatRow& Row = StatRows[CriticalChanceIndex];
		Row.DeltaValue = ClampDeltaForRow(Row, 100.f - Row.InitialValue);
	}

	const int32 CriticalDamageIndex = FindRowIndex(ULastFPSAttributeSet::GetCriticalDamagePercentAttribute());
	if (StatRows.IsValidIndex(CriticalDamageIndex))
	{
		FRuntimeStatRow& Row = StatRows[CriticalDamageIndex];
		Row.DeltaValue = ClampDeltaForRow(Row, 200.f - Row.InitialValue);
	}

	return ApplyToTarget();
}

FReply SLastFPSRuntimeStatsEditor::RefreshClicked()
{
	RefreshFromTarget();
	return FReply::Handled();
}

FReply SLastFPSRuntimeStatsEditor::ResetDeltasClicked()
{
	ResetDeltas();
	SetStatus(LOCTEXT("DeltaReset", "Reset stat deltas."));
	return FReply::Handled();
}

FReply SLastFPSRuntimeStatsEditor::AdjustDelta(int32 RowIndex, float Direction)
{
	if (StatRows.IsValidIndex(RowIndex))
	{
		FRuntimeStatRow& Row = StatRows[RowIndex];
		Row.DeltaValue = ClampDeltaForRow(Row, Row.DeltaValue + Row.StepValue * Direction);
	}

	return FReply::Handled();
}

void SLastFPSRuntimeStatsEditor::ResetDeltas()
{
	for (FRuntimeStatRow& Row : StatRows)
	{
		Row.DeltaValue = 0.f;
	}
}

int32 SLastFPSRuntimeStatsEditor::FindRowIndex(const FGameplayAttribute& Attribute) const
{
	for (int32 RowIndex = 0; RowIndex < StatRows.Num(); ++RowIndex)
	{
		if (StatRows[RowIndex].Attribute == Attribute)
		{
			return RowIndex;
		}
	}

	return INDEX_NONE;
}

float SLastFPSRuntimeStatsEditor::GetRowMaxValue(const FRuntimeStatRow& Row) const
{
	if (Row.Attribute == ULastFPSAttributeSet::GetHealthAttribute())
	{
		const int32 MaxHealthIndex = FindRowIndex(ULastFPSAttributeSet::GetMaxHealthAttribute());
		return StatRows.IsValidIndex(MaxHealthIndex) ? GetRowResult(MaxHealthIndex) : Row.MaxValue;
	}

	if (Row.Attribute == ULastFPSAttributeSet::GetStaminaAttribute())
	{
		const int32 MaxStaminaIndex = FindRowIndex(ULastFPSAttributeSet::GetMaxStaminaAttribute());
		return StatRows.IsValidIndex(MaxStaminaIndex) ? GetRowResult(MaxStaminaIndex) : Row.MaxValue;
	}

	return Row.MaxValue;
}

float SLastFPSRuntimeStatsEditor::ClampResultForRow(const FRuntimeStatRow& Row, float ResultValue) const
{
	const float MaxValue = GetRowMaxValue(Row);
	if (MaxValue == TNumericLimits<float>::Max())
	{
		return FMath::Max(ResultValue, Row.MinValue);
	}

	return FMath::Clamp(ResultValue, Row.MinValue, MaxValue);
}

float SLastFPSRuntimeStatsEditor::ClampDeltaForRow(const FRuntimeStatRow& Row, float DeltaValue) const
{
	return ClampResultForRow(Row, Row.InitialValue + DeltaValue) - Row.InitialValue;
}

float SLastFPSRuntimeStatsEditor::GetRowResult(int32 RowIndex) const
{
	if (!StatRows.IsValidIndex(RowIndex))
	{
		return 0.f;
	}

	const FRuntimeStatRow& Row = StatRows[RowIndex];
	return ClampResultForRow(Row, Row.InitialValue + Row.DeltaValue);
}

void SLastFPSRuntimeStatsEditor::ApplyRowToTarget(UAbilitySystemComponent& ASC, int32 RowIndex) const
{
	if (!StatRows.IsValidIndex(RowIndex))
	{
		return;
	}

	const FRuntimeStatRow& Row = StatRows[RowIndex];
	ASC.SetNumericAttributeBase(Row.Attribute, GetRowResult(RowIndex));
}

void SLastFPSRuntimeStatsEditor::SetStatus(const FText& NewStatus)
{
	if (StatusText)
	{
		StatusText->SetText(NewStatus);
	}
}

#undef LOCTEXT_NAMESPACE
