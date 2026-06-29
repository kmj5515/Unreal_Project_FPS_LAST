#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Widgets/SCompoundWidget.h"

class UAbilitySystemComponent;

class SLastFPSRuntimeStatsEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLastFPSRuntimeStatsEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FRuntimeStatRow
	{
		FText Label;
		FGameplayAttribute Attribute;
		float InitialValue = 0.f;
		float DeltaValue = 0.f;
		float MinValue = 0.f;
		float MaxValue = TNumericLimits<float>::Max();
		float StepValue = 1.f;
	};

	void InitializeStatRows();
	UAbilitySystemComponent* FindTargetAbilitySystem() const;
	void RefreshFromTarget();
	FReply ApplyToTarget();
	FReply ForceCriticalTest();
	FReply RefreshClicked();
	FReply ResetDeltasClicked();
	FReply AdjustDelta(int32 RowIndex, float Direction);
	TSharedRef<SWidget> BuildHeaderRow();
	TSharedRef<SWidget> BuildStatRow(int32 RowIndex);
	void ResetDeltas();
	int32 FindRowIndex(const FGameplayAttribute& Attribute) const;
	float GetRowMaxValue(const FRuntimeStatRow& Row) const;
	float ClampResultForRow(const FRuntimeStatRow& Row, float ResultValue) const;
	float ClampDeltaForRow(const FRuntimeStatRow& Row, float DeltaValue) const;
	float GetRowResult(int32 RowIndex) const;
	void ApplyRowToTarget(UAbilitySystemComponent& ASC, int32 RowIndex) const;
	void SetStatus(const FText& NewStatus);

	TArray<FRuntimeStatRow> StatRows;
	TSharedPtr<class STextBlock> StatusText;
};
