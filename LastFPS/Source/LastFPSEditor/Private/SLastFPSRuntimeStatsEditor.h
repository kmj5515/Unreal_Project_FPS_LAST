#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UAbilitySystemComponent;

class SLastFPSRuntimeStatsEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLastFPSRuntimeStatsEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	UAbilitySystemComponent* FindTargetAbilitySystem() const;
	void RefreshFromTarget();
	FReply ApplyToTarget();
	FReply ForceCriticalTest();
	FReply RefreshClicked();
	TSharedRef<SWidget> BuildStatRow(const FText& Label, float* ValuePtr, float MinValue, float MaxValue = TNumericLimits<float>::Max());
	void SetStatus(const FText& NewStatus);

	float Health = 100.f;
	float MaxHealth = 100.f;
	float Stamina = 100.f;
	float MaxStamina = 100.f;
	float AttackDamage = 10.f;
	float CriticalChance = 0.f;
	float CriticalDamagePercent = 150.f;
	float Defense = 0.f;
	float MoveSpeed = 500.f;

	TSharedPtr<class STextBlock> StatusText;
};
