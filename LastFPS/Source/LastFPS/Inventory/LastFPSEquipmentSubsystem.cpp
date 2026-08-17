#include "Inventory/LastFPSEquipmentSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Data/Tables/LastFPSExternalComponentData.h"
#include "Data/Tables/LastFPSModuleData.h"
#include "Data/Tables/LastFPSReactorData.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameplayEffect.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEquipment, Log, All);

namespace
{
	/**
	 * 카테고리별 규칙을 한 표에 모은다. 슬롯 타입이 늘어도 이 표에 한 줄을 더하면 되고,
	 * 조회·검증·합산 코드는 표를 그대로 읽으므로 분기가 흩어지지 않는다.
	 */
	struct FLastFPSEquipmentCategoryRule
	{
		ELastFPSItemType AcceptedItemType;
		FGameplayTag TableId;
	};

	FLastFPSEquipmentCategoryRule GetCategoryRule(ELastFPSEquipmentSlotType SlotType)
	{
		switch (SlotType)
		{
		case ELastFPSEquipmentSlotType::Weapon:
			// 무기는 스탯 테이블이 아니라 ULastFPSWeaponDefinition 을 참조하므로 테이블 태그가 없다.
			return { ELastFPSItemType::Weapon, FGameplayTag() };
		case ELastFPSEquipmentSlotType::Reactor:
			return { ELastFPSItemType::Reactor, LastFPSGameDataTags::Data_Table_Equipment_Reactor };
		case ELastFPSEquipmentSlotType::ExternalComponent:
			return { ELastFPSItemType::ExternalComponent, LastFPSGameDataTags::Data_Table_Equipment_External };
		case ELastFPSEquipmentSlotType::Module:
		default:
			return { ELastFPSItemType::Module, LastFPSGameDataTags::Data_Table_Economy_Module };
		}
	}

	/** 어트리뷰트 매핑 표 — 새 스탯을 추가할 때 손댈 유일한 지점이다. */
	FGameplayAttribute GetAttributeForStat(ELastFPSEquipmentStat Stat)
	{
		switch (Stat)
		{
		case ELastFPSEquipmentStat::MaxHealth:                return ULastFPSAttributeSet::GetMaxHealthAttribute();
		case ELastFPSEquipmentStat::MaxStamina:               return ULastFPSAttributeSet::GetMaxStaminaAttribute();
		case ELastFPSEquipmentStat::AttackDamage:             return ULastFPSAttributeSet::GetAttackDamageAttribute();
		case ELastFPSEquipmentStat::Defense:                  return ULastFPSAttributeSet::GetDefenseAttribute();
		case ELastFPSEquipmentStat::CriticalChance:           return ULastFPSAttributeSet::GetCriticalChanceAttribute();
		case ELastFPSEquipmentStat::CriticalDamagePercent:    return ULastFPSAttributeSet::GetCriticalDamagePercentAttribute();
		case ELastFPSEquipmentStat::PhysicalDamageMultiplier: return ULastFPSAttributeSet::GetPhysicalDamageMultiplierAttribute();
		case ELastFPSEquipmentStat::FireDamageMultiplier:     return ULastFPSAttributeSet::GetFireDamageMultiplierAttribute();
		case ELastFPSEquipmentStat::IceDamageMultiplier:      return ULastFPSAttributeSet::GetIceDamageMultiplierAttribute();
		case ELastFPSEquipmentStat::ElectricDamageMultiplier: return ULastFPSAttributeSet::GetElectricDamageMultiplierAttribute();
		case ELastFPSEquipmentStat::PoisonDamageMultiplier:   return ULastFPSAttributeSet::GetPoisonDamageMultiplierAttribute();
		case ELastFPSEquipmentStat::MoveSpeed:                return ULastFPSAttributeSet::GetMoveSpeedAttribute();
		default:                                              return FGameplayAttribute();
		}
	}

	/** 리액터·외장 부품이 공유하는 저작 형식을 합계에 옮긴다. */
	void AccumulateStatMods(const TArray<FLastFPSEquipmentStatMod>& StatMods, FLastFPSEquipmentStatTotals& OutTotals)
	{
		for (const FLastFPSEquipmentStatMod& Mod : StatMods)
		{
			OutTotals.AddStat(Mod.Stat, Mod.Value);
		}
	}
}

void ULastFPSEquipmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 보유 판정과 모듈 위임에 두 서브시스템이 필요하므로 초기화 순서를 명시한다.
	Collection.InitializeDependency<ULastFPSEconomySubsystem>();
	Collection.InitializeDependency<ULastFPSGameDataSubsystem>();
	Collection.InitializeDependency<ULastFPSLoadoutSubsystem>();

	Super::Initialize(Collection);

	WeaponSlots.Init(NAME_None, FMath::Max(1, WeaponSlotCount));
	ReactorSlots.Init(NAME_None, FMath::Max(1, ReactorSlotCount));
	ExternalComponentSlots.Init(NAME_None, FMath::Max(1, ExternalComponentSlotCount));

	if (ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout())
	{
		Loadout->OnLoadoutChanged.AddUniqueDynamic(
			this, &ULastFPSEquipmentSubsystem::HandleModuleLoadoutChanged);
	}

	ValidateEquipmentReferences();
}

void ULastFPSEquipmentSubsystem::Deinitialize()
{
	if (ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout())
	{
		Loadout->OnLoadoutChanged.RemoveDynamic(
			this, &ULastFPSEquipmentSubsystem::HandleModuleLoadoutChanged);
	}

	Super::Deinitialize();
}

void ULastFPSEquipmentSubsystem::HandleModuleLoadoutChanged()
{
	// Loadout 변경 알림에는 슬롯 정보가 없으므로 모듈 카테고리 전체 변경으로 전달한다.
	OnEquipmentChanged.Broadcast(ELastFPSEquipmentSlotType::Module, INDEX_NONE);
}

ULastFPSEconomySubsystem* ULastFPSEquipmentSubsystem::GetEconomy() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

ULastFPSLoadoutSubsystem* ULastFPSEquipmentSubsystem::GetModuleLoadout() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULastFPSLoadoutSubsystem>() : nullptr;
}

const UDataTable* ULastFPSEquipmentSubsystem::GetItemTable() const
{
	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	return GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Economy_Item) : nullptr;
}

const FLastFPSItemData* ULastFPSEquipmentSubsystem::FindItem(FName ItemRowId) const
{
	if (ItemRowId.IsNone())
	{
		return nullptr;
	}

	const UDataTable* Table = GetItemTable();
	if (!Table)
	{
		return nullptr;
	}

	static const FString Context(TEXT("ULastFPSEquipmentSubsystem::FindItem"));
	return Table->FindRow<FLastFPSItemData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false);
}

TArray<FName>* ULastFPSEquipmentSubsystem::FindOwnedSlotArray(ELastFPSEquipmentSlotType SlotType)
{
	switch (SlotType)
	{
	case ELastFPSEquipmentSlotType::Weapon:            return &WeaponSlots;
	case ELastFPSEquipmentSlotType::Reactor:           return &ReactorSlots;
	case ELastFPSEquipmentSlotType::ExternalComponent: return &ExternalComponentSlots;
	default:                                           return nullptr;
	}
}

const TArray<FName>* ULastFPSEquipmentSubsystem::FindOwnedSlotArray(ELastFPSEquipmentSlotType SlotType) const
{
	return const_cast<ULastFPSEquipmentSubsystem*>(this)->FindOwnedSlotArray(SlotType);
}

int32 ULastFPSEquipmentSubsystem::GetSlotCount(ELastFPSEquipmentSlotType SlotType) const
{
	if (SlotType == ELastFPSEquipmentSlotType::Module)
	{
		const ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout();
		return Loadout ? Loadout->GetSlotCount() : 0;
	}

	const TArray<FName>* Slots = FindOwnedSlotArray(SlotType);
	return Slots ? Slots->Num() : 0;
}

FName ULastFPSEquipmentSubsystem::GetEquippedItem(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex) const
{
	if (SlotType == ELastFPSEquipmentSlotType::Module)
	{
		const ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout();
		return Loadout ? Loadout->GetEquippedModule(SlotIndex) : NAME_None;
	}

	const TArray<FName>* Slots = FindOwnedSlotArray(SlotType);
	return (Slots && Slots->IsValidIndex(SlotIndex)) ? (*Slots)[SlotIndex] : NAME_None;
}

ELastFPSItemType ULastFPSEquipmentSubsystem::GetAcceptedItemType(ELastFPSEquipmentSlotType SlotType) const
{
	return GetCategoryRule(SlotType).AcceptedItemType;
}

bool ULastFPSEquipmentSubsystem::HasSpareCopy(
	ELastFPSEquipmentSlotType SlotType, int32 IgnoredSlotIndex, FName ItemRowId) const
{
	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	if (!Economy)
	{
		return false;
	}

	const int32 OwnedCount = Economy->GetItemCount(ItemRowId);
	if (OwnedCount <= 0)
	{
		return false;
	}

	// 같은 아이템이 다른 카테고리 슬롯에 들어갈 일은 없으므로 같은 카테고리만 센다.
	int32 UsedCount = 0;
	const int32 SlotCount = GetSlotCount(SlotType);
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		if (Index != IgnoredSlotIndex && GetEquippedItem(SlotType, Index) == ItemRowId)
		{
			++UsedCount;
		}
	}

	return UsedCount < OwnedCount;
}

bool ULastFPSEquipmentSubsystem::PassesCategoryRule(
	ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId) const
{
	switch (SlotType)
	{
	case ELastFPSEquipmentSlotType::Weapon:
	{
		// 무기 정의가 없으면 장착해도 들 무기가 없다. 참조가 비어 있는 행은 거른다.
		const FLastFPSItemData* Item = FindItem(ItemRowId);
		return Item && !Item->WeaponDefinition.IsNull();
	}
	case ELastFPSEquipmentSlotType::ExternalComponent:
	{
		UGameInstance* GameInstance = GetGameInstance();
		ULastFPSGameDataSubsystem* GameData =
			GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
		const UDataTable* ExternalTable =
			GameData ? GameData->FindTable(LastFPSGameDataTags::Data_Table_Equipment_External) : nullptr;
		if (!ExternalTable)
		{
			return false;
		}

		static const FString Context(TEXT("ULastFPSEquipmentSubsystem::PassesCategoryRule"));
		const FLastFPSExternalComponentData* Row =
			ExternalTable->FindRow<FLastFPSExternalComponentData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false);
		if (!Row)
		{
			return false;
		}

		// SlotIndex 0 은 "어느 슬롯에나" 를 뜻한다. 그 외에는 1-based 지정 슬롯과 일치해야 한다.
		return Row->SlotIndex == 0 || (Row->SlotIndex - 1) == SlotIndex;
	}
	case ELastFPSEquipmentSlotType::Reactor:
	case ELastFPSEquipmentSlotType::Module:
	default:
		return true;
	}
}

bool ULastFPSEquipmentSubsystem::CanEquip(
	ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId) const
{
	if (ItemRowId.IsNone() || SlotIndex < 0 || SlotIndex >= GetSlotCount(SlotType))
	{
		return false;
	}

	// 모듈은 캐파 규칙까지 포함해 기존 서브시스템이 판단해야 한다.
	if (SlotType == ELastFPSEquipmentSlotType::Module)
	{
		const ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout();
		return Loadout && Loadout->CanEquip(SlotIndex, ItemRowId);
	}

	const FLastFPSItemData* Item = FindItem(ItemRowId);
	if (!Item || Item->ItemType != GetAcceptedItemType(SlotType))
	{
		return false;
	}

	if (!HasSpareCopy(SlotType, SlotIndex, ItemRowId))
	{
		return false;
	}

	return PassesCategoryRule(SlotType, SlotIndex, ItemRowId);
}

bool ULastFPSEquipmentSubsystem::TryEquip(
	ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName ItemRowId)
{
	if (!CanEquip(SlotType, SlotIndex, ItemRowId))
	{
		return false;
	}

	if (SlotType == ELastFPSEquipmentSlotType::Module)
	{
		ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout();
		if (!Loadout || !Loadout->TryEquip(SlotIndex, ItemRowId))
		{
			return false;
		}

		return true;
	}

	TArray<FName>* Slots = FindOwnedSlotArray(SlotType);
	if (!Slots || !Slots->IsValidIndex(SlotIndex))
	{
		return false;
	}

	(*Slots)[SlotIndex] = ItemRowId;
	OnEquipmentChanged.Broadcast(SlotType, SlotIndex);
	return true;
}

void ULastFPSEquipmentSubsystem::Unequip(ELastFPSEquipmentSlotType SlotType, int32 SlotIndex)
{
	if (SlotType == ELastFPSEquipmentSlotType::Module)
	{
		ULastFPSLoadoutSubsystem* Loadout = GetModuleLoadout();
		if (Loadout && !Loadout->GetEquippedModule(SlotIndex).IsNone())
		{
			Loadout->Unequip(SlotIndex);
		}
		return;
	}

	TArray<FName>* Slots = FindOwnedSlotArray(SlotType);
	if (Slots && Slots->IsValidIndex(SlotIndex) && !(*Slots)[SlotIndex].IsNone())
	{
		(*Slots)[SlotIndex] = NAME_None;
		OnEquipmentChanged.Broadcast(SlotType, SlotIndex);
	}
}

ULastFPSWeaponDefinition* ULastFPSEquipmentSubsystem::GetWeaponDefinitionForSlot(int32 SlotIndex) const
{
	const FName ItemRowId = GetEquippedItem(ELastFPSEquipmentSlotType::Weapon, SlotIndex);
	const FLastFPSItemData* Item = FindItem(ItemRowId);
	if (!Item || Item->WeaponDefinition.IsNull())
	{
		return nullptr;
	}

	// 장착 시점에는 무기를 즉시 들어야 하므로 동기 로드를 감수한다.
	// 배틀 맵 진입 전 미리 로드하려면 DestinationContentSet 에 무기 정의를 포함시킬 것.
	return Item->WeaponDefinition.LoadSynchronous();
}

ULastFPSWeaponDefinition* ULastFPSEquipmentSubsystem::FindWeaponDefinitionForItem(const FName ItemRowId) const
{
	const FLastFPSItemData* Item = FindItem(ItemRowId);
	if (!Item || Item->WeaponDefinition.IsNull())
	{
		return nullptr;
	}

	// 장착 시점에는 무기를 즉시 들어야 하므로 동기 로드를 감수한다(GetWeaponDefinitionForSlot 와 동일 계약).
	return Item->WeaponDefinition.LoadSynchronous();
}

TArray<FLastFPSEquippedSlot> ULastFPSEquipmentSubsystem::BuildEquippedSlots() const
{
	TArray<FLastFPSEquippedSlot> Slots;

	for (int32 TypeIndex = 0; TypeIndex < static_cast<int32>(ELastFPSEquipmentSlotType::Count); ++TypeIndex)
	{
		const ELastFPSEquipmentSlotType SlotType = static_cast<ELastFPSEquipmentSlotType>(TypeIndex);
		const int32 SlotCount = GetSlotCount(SlotType);
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			const FName ItemRowId = GetEquippedItem(SlotType, SlotIndex);
			if (ItemRowId.IsNone())
			{
				// 빈 슬롯은 보낼 이유가 없다. 무기는 SlotIndex 로 주/보조를 구분하므로 인덱스를 함께 담는다.
				continue;
			}

			FLastFPSEquippedSlot& Entry = Slots.AddDefaulted_GetRef();
			Entry.SlotType = SlotType;
			Entry.SlotIndex = SlotIndex;
			Entry.ItemRowId = ItemRowId;
		}
	}

	return Slots;
}

bool ULastFPSEquipmentSubsystem::IsSubmittedSlotWellFormed(const FLastFPSEquippedSlot& Entry) const
{
	if (Entry.ItemRowId.IsNone()
		|| Entry.SlotIndex < 0
		|| Entry.SlotIndex >= GetSlotCount(Entry.SlotType))
	{
		return false;
	}

	const FLastFPSItemData* Item = FindItem(Entry.ItemRowId);
	return Item && Item->ItemType == GetAcceptedItemType(Entry.SlotType);
}

FLastFPSEquipmentStatTotals ULastFPSEquipmentSubsystem::ComputeTotalsForSlots(
	const TArray<FLastFPSEquippedSlot>& Slots) const
{
	FLastFPSEquipmentStatTotals Totals;

	for (const FLastFPSEquippedSlot& Entry : Slots)
	{
		AccumulateSlotStats(Entry.SlotType, Entry.ItemRowId, Totals);
	}

	return Totals;
}

bool ULastFPSEquipmentSubsystem::HasEquippedWeapon() const
{
	for (int32 SlotIndex = 0; SlotIndex < WeaponSlots.Num(); ++SlotIndex)
	{
		if (GetWeaponDefinitionForSlot(SlotIndex))
		{
			return true;
		}
	}

	return false;
}

void ULastFPSEquipmentSubsystem::AccumulateSlotStats(
	ELastFPSEquipmentSlotType SlotType, FName ItemRowId, FLastFPSEquipmentStatTotals& OutTotals) const
{
	if (ItemRowId.IsNone())
	{
		return;
	}

	// 무기는 스탯 보정이 아니라 장착 무기 자체가 성능이므로 합계에 기여하지 않는다.
	const FLastFPSEquipmentCategoryRule Rule = GetCategoryRule(SlotType);
	if (!Rule.TableId.IsValid())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	const UDataTable* Table = GameData ? GameData->FindTable(Rule.TableId) : nullptr;
	if (!Table)
	{
		return;
	}

	static const FString Context(TEXT("ULastFPSEquipmentSubsystem::AccumulateSlotStats"));

	switch (SlotType)
	{
	case ELastFPSEquipmentSlotType::Reactor:
		if (const FLastFPSReactorData* Row =
			Table->FindRow<FLastFPSReactorData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false))
		{
			AccumulateStatMods(Row->StatMods, OutTotals);
		}
		break;

	case ELastFPSEquipmentSlotType::ExternalComponent:
		if (const FLastFPSExternalComponentData* Row =
			Table->FindRow<FLastFPSExternalComponentData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false))
		{
			AccumulateStatMods(Row->StatMods, OutTotals);
		}
		break;

	case ELastFPSEquipmentSlotType::Module:
		// 모듈만 기존 열거형으로 저작되어 있어 공용 계약으로 변환해 합친다.
		if (const FLastFPSModuleData* Row =
			Table->FindRow<FLastFPSModuleData>(ItemRowId, Context, /*bWarnIfRowMissing=*/false))
		{
			for (const FLastFPSModuleStatMod& Mod : Row->StatMods)
			{
				ELastFPSEquipmentStat Stat;
				if (LastFPSEquipmentStats::ToEquipmentStat(Mod.Stat, Stat))
				{
					OutTotals.AddStat(Stat, Mod.Value);
				}
			}
		}
		break;

	default:
		break;
	}
}

FLastFPSEquipmentStatTotals ULastFPSEquipmentSubsystem::ComputeCategoryTotals(
	ELastFPSEquipmentSlotType SlotType) const
{
	FLastFPSEquipmentStatTotals Totals;

	const int32 SlotCount = GetSlotCount(SlotType);
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		AccumulateSlotStats(SlotType, GetEquippedItem(SlotType, Index), Totals);
	}

	return Totals;
}

FLastFPSEquipmentStatTotals ULastFPSEquipmentSubsystem::ComputeTotals() const
{
	FLastFPSEquipmentStatTotals Totals;

	for (int32 TypeIndex = 0; TypeIndex < static_cast<int32>(ELastFPSEquipmentSlotType::Count); ++TypeIndex)
	{
		Totals.Append(ComputeCategoryTotals(static_cast<ELastFPSEquipmentSlotType>(TypeIndex)));
	}

	return Totals;
}

FLastFPSEquipmentStatTotals ULastFPSEquipmentSubsystem::ComputeTotalsWithCandidate(
	ELastFPSEquipmentSlotType SlotType, int32 SlotIndex, FName CandidateItemRowId) const
{
	FLastFPSEquipmentStatTotals Totals;

	for (int32 TypeIndex = 0; TypeIndex < static_cast<int32>(ELastFPSEquipmentSlotType::Count); ++TypeIndex)
	{
		const ELastFPSEquipmentSlotType CurrentType = static_cast<ELastFPSEquipmentSlotType>(TypeIndex);
		if (CurrentType != SlotType)
		{
			Totals.Append(ComputeCategoryTotals(CurrentType));
			continue;
		}

		// 대상 카테고리만 해당 슬롯을 후보로 바꿔 넣고 나머지 슬롯은 현재 값을 그대로 쓴다.
		const int32 SlotCount = GetSlotCount(CurrentType);
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			const FName RowId = (Index == SlotIndex) ? CandidateItemRowId : GetEquippedItem(CurrentType, Index);
			AccumulateSlotStats(CurrentType, RowId, Totals);
		}
	}

	return Totals;
}

FActiveGameplayEffectHandle ULastFPSEquipmentSubsystem::ApplyStatTotalsToAbilitySystem(
	UAbilitySystemComponent* ASC, const FLastFPSEquipmentStatTotals& Bonus) const
{
	if (!ASC || !Bonus.HasAny())
	{
		return FActiveGameplayEffectHandle();
	}

	// 런타임 Infinite GE 하나에 모든 카테고리 보정을 담아 베이스 스탯 위에 얹는다.
	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_EquipmentStats")));
	GE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	for (int32 StatIndex = 0; StatIndex < static_cast<int32>(ELastFPSEquipmentStat::Count); ++StatIndex)
	{
		const ELastFPSEquipmentStat Stat = static_cast<ELastFPSEquipmentStat>(StatIndex);
		const float Value = Bonus.GetStat(Stat);
		if (FMath::IsNearlyZero(Value))
		{
			continue;
		}

		const FGameplayAttribute Attribute = GetAttributeForStat(Stat);
		if (!Attribute.IsValid())
		{
			UE_LOG(LogLastFPSEquipment, Warning,
				TEXT("장비 스탯 '%s' 에 대응하는 어트리뷰트가 없어 적용을 건너뜁니다."),
				*LastFPSEquipmentStats::GetDisplayName(Stat).ToString());
			continue;
		}

		const int32 ModifierIndex = GE->Modifiers.Num();
		GE->Modifiers.SetNum(ModifierIndex + 1);
		FGameplayModifierInfo& Info = GE->Modifiers[ModifierIndex];
		Info.Attribute = Attribute;
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
	}

	if (GE->Modifiers.Num() == 0)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	return ASC->ApplyGameplayEffectToSelf(GE, 1.f, Context);
}

void ULastFPSEquipmentSubsystem::ValidateEquipmentReferences() const
{
#if !UE_BUILD_SHIPPING
	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	if (!Economy || !Economy->IsItemTableConfigured())
	{
		// 아이템 테이블 로드 실패 시 모든 참조를 오류로 판단하는 오탐을 막는다.
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULastFPSGameDataSubsystem* GameData =
		GameInstance ? GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>() : nullptr;
	if (!GameData)
	{
		return;
	}

	static const FString Context(TEXT("ULastFPSEquipmentSubsystem::ValidateEquipmentReferences"));

	if (const UDataTable* ReactorTable = GameData->FindTable(LastFPSGameDataTags::Data_Table_Equipment_Reactor))
	{
		ReactorTable->ForeachRow<FLastFPSReactorData>(Context,
			[Economy](const FName& RowName, const FLastFPSReactorData&)
			{
				if (!Economy->HasItemDefinition(RowName))
				{
					UE_LOG(LogLastFPSEquipment, Error,
						TEXT("[Reactor] 행 '%s' 에 대응하는 DT_ItemData 행이 없음 — 인벤토리에 표시/장착 불가."),
						*RowName.ToString());
				}
			});
	}

	if (const UDataTable* ExternalTable = GameData->FindTable(LastFPSGameDataTags::Data_Table_Equipment_External))
	{
		ExternalTable->ForeachRow<FLastFPSExternalComponentData>(Context,
			[Economy](const FName& RowName, const FLastFPSExternalComponentData&)
			{
				if (!Economy->HasItemDefinition(RowName))
				{
					UE_LOG(LogLastFPSEquipment, Error,
						TEXT("[ExternalComponent] 행 '%s' 에 대응하는 DT_ItemData 행이 없음 — 인벤토리에 표시/장착 불가."),
						*RowName.ToString());
				}
			});
	}
#endif
}
