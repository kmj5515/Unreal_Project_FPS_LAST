#include "Inventory/LastFPSLoadoutSubsystem.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/DataTable.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Economy/LastFPSEconomySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSLoadout, Log, All);

void ULastFPSLoadoutSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Economy 를 먼저 초기화하도록 순서를 명시(모듈 검증이 Economy->HasItemDefinition 를 사용).
	Collection.InitializeDependency<ULastFPSEconomySubsystem>();

	Super::Initialize(Collection);

	// 슬롯 배열을 빈 칸(NAME_None)으로 초기화
	EquippedModules.Init(NAME_None, FMath::Max(1, SlotCount));

	ValidateModuleReferences();
}

void ULastFPSLoadoutSubsystem::ValidateModuleReferences() const
{
#if !UE_BUILD_SHIPPING
	const UDataTable* Table = ModuleTable.LoadSynchronous();
	if (!Table)
	{
		UE_LOG(LogLastFPSLoadout, Warning,
			TEXT("ModuleTable(DT_ModuleData) 미설정 — 모듈 참조 검증을 건너뜀."));
		return;
	}

	const ULastFPSEconomySubsystem* Economy = GetEconomy();
	if (!Economy || !Economy->IsItemTableConfigured())
	{
		// ItemTable 미설정 시 HasItemDefinition 이 전부 false 라 오탐이 되므로 검증을 건너뛴다.
		return;
	}

	int32 Broken = 0;
	static const FString Ctx(TEXT("ULastFPSLoadoutSubsystem::ValidateModuleReferences"));
	Table->ForeachRow<FLastFPSModuleData>(Ctx,
		[Economy, &Broken](const FName& RowName, const FLastFPSModuleData&)
		{
			// ItemTable 미설정이면 Economy 가 검증 불가라 false 를 돌려주므로, 그 경우는 건너뛴다.
			if (!Economy->HasItemDefinition(RowName))
			{
				++Broken;
				UE_LOG(LogLastFPSLoadout, Error,
					TEXT("[Module] 행 '%s' 에 대응하는 DT_ItemData 행이 없음 — 인벤토리에 표시/장착 불가."),
					*RowName.ToString());
			}
		});

	UE_LOG(LogLastFPSLoadout, Log, TEXT("모듈 테이블 참조 검증: 깨진 참조 %d건."), Broken);
#endif
}

ULastFPSEconomySubsystem* ULastFPSLoadoutSubsystem::GetEconomy() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULastFPSEconomySubsystem>() : nullptr;
}

const FLastFPSModuleData* ULastFPSLoadoutSubsystem::FindModule(FName ModuleRowId) const
{
	if (ModuleRowId.IsNone())
	{
		return nullptr;
	}

	const UDataTable* Table = ModuleTable.LoadSynchronous();
	if (!Table)
	{
		return nullptr;
	}

	static const FString Context(TEXT("ULastFPSLoadoutSubsystem::FindModule"));
	return Table->FindRow<FLastFPSModuleData>(ModuleRowId, Context, /*bWarnIfRowMissing=*/false);
}

FName ULastFPSLoadoutSubsystem::GetEquippedModule(int32 SlotIndex) const
{
	return EquippedModules.IsValidIndex(SlotIndex) ? EquippedModules[SlotIndex] : NAME_None;
}

int32 ULastFPSLoadoutSubsystem::GetUsedCapacity() const
{
	int32 Used = 0;
	for (const FName& RowId : EquippedModules)
	{
		if (const FLastFPSModuleData* Module = FindModule(RowId))
		{
			Used += FMath::Max(0, Module->CapacityCost);
		}
	}
	return Used;
}

bool ULastFPSLoadoutSubsystem::CanEquip(int32 SlotIndex, FName ModuleRowId) const
{
	if (!EquippedModules.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FLastFPSModuleData* NewModule = FindModule(ModuleRowId);
	if (!NewModule)
	{
		return false;
	}

	// 보유 여부 (DT_ItemData 와 동일 행 이름으로 EconomySubsystem 에서 판정)
	if (const ULastFPSEconomySubsystem* Economy = GetEconomy())
	{
		if (Economy->GetItemCount(ModuleRowId) <= 0)
		{
			return false;
		}
	}

	// 이 슬롯에 들어 있던 모듈 코스트는 빼고, 새 모듈 코스트를 더해 한도 검사
	int32 CurrentSlotCost = 0;
	if (const FLastFPSModuleData* Existing = FindModule(EquippedModules[SlotIndex]))
	{
		CurrentSlotCost = FMath::Max(0, Existing->CapacityCost);
	}

	const int32 Prospective = GetUsedCapacity() - CurrentSlotCost + FMath::Max(0, NewModule->CapacityCost);
	return Prospective <= MaxCapacity;
}

bool ULastFPSLoadoutSubsystem::TryEquip(int32 SlotIndex, FName ModuleRowId)
{
	if (!CanEquip(SlotIndex, ModuleRowId))
	{
		return false;
	}

	EquippedModules[SlotIndex] = ModuleRowId;
	OnLoadoutChanged.Broadcast();
	return true;
}

void ULastFPSLoadoutSubsystem::Unequip(int32 SlotIndex)
{
	if (EquippedModules.IsValidIndex(SlotIndex) && !EquippedModules[SlotIndex].IsNone())
	{
		EquippedModules[SlotIndex] = NAME_None;
		OnLoadoutChanged.Broadcast();
	}
}

FLastFPSModuleStatTotals ULastFPSLoadoutSubsystem::ComputeBonus() const
{
	FLastFPSModuleStatTotals Totals;
	for (const FName& RowId : EquippedModules)
	{
		const FLastFPSModuleData* Module = FindModule(RowId);
		if (!Module)
		{
			continue;
		}

		for (const FLastFPSModuleStatMod& Mod : Module->StatMods)
		{
			switch (Mod.Stat)
			{
			case ELastFPSModuleStat::MaxHealth:    Totals.MaxHealth    += Mod.Value; break;
			case ELastFPSModuleStat::MaxStamina:   Totals.MaxStamina   += Mod.Value; break;
			case ELastFPSModuleStat::AttackDamage: Totals.AttackDamage += Mod.Value; break;
			case ELastFPSModuleStat::Defense:      Totals.Defense      += Mod.Value; break;
			case ELastFPSModuleStat::PhysicalDamageMultiplier: Totals.PhysicalDamageMultiplier += Mod.Value; break;
			case ELastFPSModuleStat::FireDamageMultiplier:     Totals.FireDamageMultiplier     += Mod.Value; break;
			case ELastFPSModuleStat::IceDamageMultiplier:      Totals.IceDamageMultiplier      += Mod.Value; break;
			case ELastFPSModuleStat::ElectricDamageMultiplier: Totals.ElectricDamageMultiplier += Mod.Value; break;
			case ELastFPSModuleStat::PoisonDamageMultiplier:   Totals.PoisonDamageMultiplier   += Mod.Value; break;
			case ELastFPSModuleStat::MoveSpeed:    Totals.MoveSpeed    += Mod.Value; break;
			default: break;
			}
		}
	}
	return Totals;
}

void ULastFPSLoadoutSubsystem::ApplyToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}

	const FLastFPSModuleStatTotals Bonus = ComputeBonus();
	if (!Bonus.HasAny())
	{
		return;
	}

	// 런타임 Infinite GE 를 구성해 베이스 스탯 위에 가산 보정으로 얹는다.
	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("GE_ModuleStats")));
	GE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	auto AddModifier = [GE](const FGameplayAttribute& Attribute, float Value)
	{
		if (FMath::IsNearlyZero(Value))
		{
			return;
		}
		const int32 Index = GE->Modifiers.Num();
		GE->Modifiers.SetNum(Index + 1);
		FGameplayModifierInfo& Info = GE->Modifiers[Index];
		Info.Attribute = Attribute;
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
	};

	AddModifier(ULastFPSAttributeSet::GetMaxHealthAttribute(),    Bonus.MaxHealth);
	AddModifier(ULastFPSAttributeSet::GetMaxStaminaAttribute(),   Bonus.MaxStamina);
	AddModifier(ULastFPSAttributeSet::GetAttackDamageAttribute(), Bonus.AttackDamage);
	AddModifier(ULastFPSAttributeSet::GetDefenseAttribute(),      Bonus.Defense);
	AddModifier(ULastFPSAttributeSet::GetPhysicalDamageMultiplierAttribute(), Bonus.PhysicalDamageMultiplier);
	AddModifier(ULastFPSAttributeSet::GetFireDamageMultiplierAttribute(),     Bonus.FireDamageMultiplier);
	AddModifier(ULastFPSAttributeSet::GetIceDamageMultiplierAttribute(),      Bonus.IceDamageMultiplier);
	AddModifier(ULastFPSAttributeSet::GetElectricDamageMultiplierAttribute(), Bonus.ElectricDamageMultiplier);
	AddModifier(ULastFPSAttributeSet::GetPoisonDamageMultiplierAttribute(),   Bonus.PoisonDamageMultiplier);
	AddModifier(ULastFPSAttributeSet::GetMoveSpeedAttribute(),    Bonus.MoveSpeed);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	ASC->ApplyGameplayEffectToSelf(GE, 1.f, Context);
}
