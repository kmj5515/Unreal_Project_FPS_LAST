#include "AbilitySystem/Status/LastFPSStatusEffectDataSubsystem.h"

#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStatusEffectData, Log, All);

void ULastFPSStatusEffectDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DataByStatusTag.Reset();
	LoadedTable = StatusEffectUITable.LoadSynchronous();
	if (!LoadedTable)
	{
		if (!StatusEffectUITable.IsNull())
		{
			UE_LOG(LogLastFPSStatusEffectData, Error,
				TEXT("상태 효과 UI 데이터 테이블 '%s'을 로드하지 못했습니다."),
				*StatusEffectUITable.ToString());
		}
		return;
	}

	const FGameplayTag StatusRootTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Status")), false);
	for (const FName RowName : LoadedTable->GetRowNames())
	{
		const FLastFPSStatusEffectUIData* StatusEffect = LoadedTable->FindRow<FLastFPSStatusEffectUIData>(
			RowName, TEXT("상태 효과 UI 데이터 초기화"), false);
		if (!StatusEffect)
		{
			UE_LOG(LogLastFPSStatusEffectData, Error,
				TEXT("데이터 테이블 '%s'의 행 '%s'을 읽지 못했습니다."),
				*GetNameSafe(LoadedTable), *RowName.ToString());
			continue;
		}

		if (!StatusEffect->StatusTag.IsValid()
			|| (StatusRootTag.IsValid() && !StatusEffect->StatusTag.MatchesTag(StatusRootTag)))
		{
			UE_LOG(LogLastFPSStatusEffectData, Error,
				TEXT("데이터 테이블 '%s'의 행 '%s'에 유효하지 않은 Status 하위 태그가 있습니다."),
				*GetNameSafe(LoadedTable), *RowName.ToString());
			continue;
		}
		if (DataByStatusTag.Contains(StatusEffect->StatusTag))
		{
			UE_LOG(LogLastFPSStatusEffectData, Error,
				TEXT("데이터 테이블 '%s'의 행 '%s'에 상태 효과 태그 '%s'가 중복되었습니다."),
				*GetNameSafe(LoadedTable), *RowName.ToString(), *StatusEffect->StatusTag.ToString());
			continue;
		}
		if (StatusEffect->Overlay.bEnabled && StatusEffect->Overlay.Material.IsNull())
		{
			UE_LOG(LogLastFPSStatusEffectData, Error,
				TEXT("데이터 테이블 '%s'의 행 '%s'은 Overlay가 활성화됐지만 Material이 비어 있습니다."),
				*GetNameSafe(LoadedTable), *RowName.ToString());
		}

		DataByStatusTag.Add(StatusEffect->StatusTag, StatusEffect);
	}
}

void ULastFPSStatusEffectDataSubsystem::Deinitialize()
{
	DataByStatusTag.Reset();
	LoadedTable = nullptr;
	Super::Deinitialize();
}

const FLastFPSStatusEffectUIData* ULastFPSStatusEffectDataSubsystem::FindStatusEffectUIData(
	const FGameplayTag StatusTag) const
{
	const FLastFPSStatusEffectUIData* const* Found = DataByStatusTag.Find(StatusTag);
	return Found ? *Found : nullptr;
}

bool ULastFPSStatusEffectDataSubsystem::GetStatusEffectUIData(
	const FGameplayTag StatusTag,
	FLastFPSStatusEffectUIData& OutData) const
{
	const FLastFPSStatusEffectUIData* Found = FindStatusEffectUIData(StatusTag);
	if (!Found)
	{
		return false;
	}

	OutData = *Found;
	return true;
}

void ULastFPSStatusEffectDataSubsystem::GetVisibleStatusEffects(
	const FGameplayTagContainer& OwnedTags,
	TArray<FLastFPSStatusEffectUIData>& OutData) const
{
	OutData.Reset();
	for (const TPair<FGameplayTag, const FLastFPSStatusEffectUIData*>& Entry : DataByStatusTag)
	{
		if (Entry.Value && Entry.Value->bShowOnHUD && OwnedTags.HasTagExact(Entry.Key))
		{
			OutData.Add(*Entry.Value);
		}
	}

	OutData.Sort([](const FLastFPSStatusEffectUIData& Left, const FLastFPSStatusEffectUIData& Right)
	{
		if (Left.DisplayPriority != Right.DisplayPriority)
		{
			return Left.DisplayPriority > Right.DisplayPriority;
		}
		return Left.StatusTag.GetTagName().Compare(Right.StatusTag.GetTagName()) < 0;
	});
}

void ULastFPSStatusEffectDataSubsystem::GetRegisteredStatusTags(FGameplayTagContainer& OutTags) const
{
	OutTags.Reset();
	for (const TPair<FGameplayTag, const FLastFPSStatusEffectUIData*>& Entry : DataByStatusTag)
	{
		if (Entry.Key.IsValid())
		{
			OutTags.AddTag(Entry.Key);
		}
	}
}

void ULastFPSStatusEffectDataSubsystem::GetStatusOverlayEntries(
	TArray<const FLastFPSStatusEffectUIData*>& OutEntries) const
{
	OutEntries.Reset();
	for (const TPair<FGameplayTag, const FLastFPSStatusEffectUIData*>& Entry : DataByStatusTag)
	{
		if (Entry.Value && Entry.Value->Overlay.bEnabled && !Entry.Value->Overlay.Material.IsNull())
		{
			OutEntries.Add(Entry.Value);
		}
	}
}

void ULastFPSStatusEffectDataSubsystem::GetStatusAnimationEntries(
	TArray<const FLastFPSStatusEffectUIData*>& OutEntries) const
{
	OutEntries.Reset();
	for (const TPair<FGameplayTag, const FLastFPSStatusEffectUIData*>& Entry : DataByStatusTag)
	{
		if (Entry.Value
			&& Entry.Value->AnimationPolicy != ELastFPSStatusAnimationPolicy::None)
		{
			OutEntries.Add(Entry.Value);
		}
	}
}

TSoftObjectPtr<UTexture2D> ULastFPSStatusEffectDataSubsystem::GetFallbackIcon() const
{
	return FallbackIcon;
}
