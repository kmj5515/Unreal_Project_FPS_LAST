#include "UI/HUD/LastFPSStatusEffectListWidget.h"

#include "AbilitySystem/Status/LastFPSStatusEffectDataSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "UI/HUD/LastFPSStatusEffectIconWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStatusEffectList, Log, All);

void ULastFPSStatusEffectListWidget::NativeDestruct()
{
	UninitializeFromAbilitySystem();
	Super::NativeDestruct();
}

void ULastFPSStatusEffectListWidget::InitializeWithAbilitySystem(
	UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (AbilitySystemComponent.Get() == InAbilitySystemComponent)
	{
		RefreshEntries();
		return;
	}

	UninitializeFromAbilitySystem();
	AbilitySystemComponent = InAbilitySystemComponent;
	if (!InAbilitySystemComponent)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const ULastFPSStatusEffectDataSubsystem* DataSubsystem =
			GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>())
		{
			FGameplayTagContainer RegisteredTags;
			DataSubsystem->GetRegisteredStatusTags(RegisteredTags);
			for (const FGameplayTag StatusTag : RegisteredTags)
			{
				FDelegateHandle DelegateHandle = InAbilitySystemComponent->RegisterGameplayTagEvent(
					StatusTag, EGameplayTagEventType::AnyCountChange).AddUObject(
						this, &ULastFPSStatusEffectListWidget::HandleStatusTagChanged);
				StatusTagDelegateHandles.Add(StatusTag, DelegateHandle);
			}
		}
	}
	RefreshEntries();
}

void ULastFPSStatusEffectListWidget::UninitializeFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Entry : StatusTagDelegateHandles)
		{
			ASC->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::AnyCountChange).Remove(Entry.Value);
		}
	}

	StatusTagDelegateHandles.Reset();
	AbilitySystemComponent.Reset();
	ActiveIconWidgets.Reset();
	if (StatusEffectContainer)
	{
		StatusEffectContainer->ClearChildren();
	}
}

void ULastFPSStatusEffectListWidget::HandleStatusTagChanged(
	const FGameplayTag StatusTag,
	const int32 NewCount)
{
	(void)StatusTag;
	(void)NewCount;
	RefreshEntries();
}

void ULastFPSStatusEffectListWidget::RefreshEntries()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSStatusEffectDataSubsystem* DataSubsystem =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSStatusEffectDataSubsystem>() : nullptr;
	if (!ASC || !DataSubsystem)
	{
		return;
	}
	if (!StatusEffectContainer || !StatusEffectIconWidgetClass)
	{
		if (!bConfigurationWarningLogged)
		{
			UE_LOG(LogLastFPSStatusEffectList, Warning,
				TEXT("상태 효과 목록 위젯 '%s'의 StatusEffectContainer 또는 StatusEffectIconWidgetClass가 설정되지 않았습니다."),
				*GetNameSafe(this));
			bConfigurationWarningLogged = true;
		}
		return;
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);
	TArray<FLastFPSStatusEffectUIData> VisibleEffects;
	DataSubsystem->GetVisibleStatusEffects(OwnedTags, VisibleEffects);

	TSet<FGameplayTag> VisibleTags;
	for (FLastFPSStatusEffectUIData& EffectData : VisibleEffects)
	{
		VisibleTags.Add(EffectData.StatusTag);
		if (EffectData.Icon.IsNull())
		{
			EffectData.Icon = DataSubsystem->GetFallbackIcon();
		}

		ULastFPSStatusEffectIconWidget* IconWidget = nullptr;
		if (TObjectPtr<ULastFPSStatusEffectIconWidget>* Existing =
			ActiveIconWidgets.Find(EffectData.StatusTag))
		{
			IconWidget = Existing->Get();
		}
		if (!IconWidget)
		{
			IconWidget = CreateWidget<ULastFPSStatusEffectIconWidget>(
				GetOwningPlayer(), StatusEffectIconWidgetClass);
			if (!IconWidget)
			{
				UE_LOG(LogLastFPSStatusEffectList, Error,
					TEXT("상태 효과 태그 '%s'의 아이콘 위젯을 생성하지 못했습니다."),
					*EffectData.StatusTag.ToString());
				continue;
			}
			IconWidget->InitializeStatusEffect(EffectData);
			ActiveIconWidgets.Add(EffectData.StatusTag, IconWidget);
		}

		// 제거 후 다시 추가하면 데이터의 우선순위 순서대로 패널 슬롯이 정렬된다.
		StatusEffectContainer->RemoveChild(IconWidget);
		StatusEffectContainer->AddChild(IconWidget);
	}

	TArray<FGameplayTag> TagsToRemove;
	ActiveIconWidgets.GetKeys(TagsToRemove);
	for (const FGameplayTag StatusTag : TagsToRemove)
	{
		if (VisibleTags.Contains(StatusTag))
		{
			continue;
		}

		if (ULastFPSStatusEffectIconWidget* IconWidget = ActiveIconWidgets.FindRef(StatusTag))
		{
			StatusEffectContainer->RemoveChild(IconWidget);
		}
		ActiveIconWidgets.Remove(StatusTag);
	}

	UpdateRuntimeStates();
}

void ULastFPSStatusEffectListWidget::UpdateRuntimeStates()
{
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}

	for (const TPair<FGameplayTag, TObjectPtr<ULastFPSStatusEffectIconWidget>>& Entry : ActiveIconWidgets)
	{
		if (!Entry.Value)
		{
			continue;
		}

		float TimeRemaining = -1.f;
		float Duration = -1.f;
		int32 StackCount = 1;
		QueryRuntimeState(Entry.Key, TimeRemaining, Duration, StackCount);
		Entry.Value->UpdateRuntimeState(TimeRemaining, Duration, StackCount);
	}
}

void ULastFPSStatusEffectListWidget::QueryRuntimeState(
	const FGameplayTag StatusTag,
	float& OutRemaining,
	float& OutDuration,
	int32& OutStacks) const
{
	OutRemaining = -1.f;
	OutDuration = -1.f;
	OutStacks = 1;

	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !StatusTag.IsValid())
	{
		return;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(StatusTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags);
	const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	for (const TPair<float, float>& Time : Times)
	{
		if (Time.Value > KINDA_SMALL_NUMBER && Time.Key >= 0.f
			&& (OutRemaining < 0.f || Time.Key > OutRemaining))
		{
			OutRemaining = Time.Key;
			OutDuration = Time.Value;
		}
	}

	OutStacks = FMath::Max(ASC->GetAggregatedStackCount(Query), 1);
}
