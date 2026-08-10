#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Effects/GE_Cooldown.h"
#include "AbilitySystem/Effects/GE_HealInstant.h"
#include "AbilitySystem/Effects/GE_MoveSpeedBuff.h"
#include "AbilitySystem/Effects/GE_StatusFreeze.h"
#include "AbilitySystem/Effects/GE_StatusSlow.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "Data/Tables/LastFPSCharacterSkillData.h"
#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace LastFPSCooldownAutomation
{
	const FName ProbePassiveSkillId(TEXT("Viola_ProbePassive"));
	constexpr float ActiveCooldownLowerBound = 0.f;

	struct FGameDataContext
	{
		FGameDataContext()
			: GameInstance(NewObject<UGameInstance>())
			, GameData(NewObject<ULastFPSGameDataSubsystem>(GameInstance.Get()))
		{
		}

		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<ULastFPSGameDataSubsystem> GameData;
	};

	bool LoadCooldownTables(
		FAutomationTestBase& Test,
		FGameDataContext& Context,
		UDataTable*& OutCharacterSkillTable,
		UDataTable*& OutSkillBalanceTable)
	{
		if (!Test.TestNotNull(TEXT("게임 데이터 계약 객체"), Context.GameData.Get()))
		{
			return false;
		}

		OutCharacterSkillTable = Context.GameData->FindTable(
			LastFPSGameDataTags::Data_Table_Character_Skill);
		OutSkillBalanceTable = Context.GameData->FindTable(
			LastFPSGameDataTags::Data_Table_Balance_Skill);

		const bool bHasCharacterSkillTable = Test.TestNotNull(
			TEXT("Data.Table.Character.Skill 등록 테이블"), OutCharacterSkillTable);
		const bool bHasSkillBalanceTable = Test.TestNotNull(
			TEXT("Data.Table.Balance.Skill 등록 테이블"), OutSkillBalanceTable);
		if (!bHasCharacterSkillTable || !bHasSkillBalanceTable)
		{
			return false;
		}

		const bool bHasCharacterSkillRows = Test.TestTrue(
			TEXT("캐릭터 스킬 테이블 행 구조"),
			OutCharacterSkillTable->GetRowStruct()
				&& OutCharacterSkillTable->GetRowStruct()->IsChildOf(
					FLastFPSCharacterSkillData::StaticStruct()));
		const bool bHasSkillBalanceRows = Test.TestTrue(
			TEXT("스킬 밸런스 테이블 행 구조"),
			OutSkillBalanceTable->GetRowStruct()
				&& OutSkillBalanceTable->GetRowStruct()->IsChildOf(
					FLastFPSSkillBalanceData::StaticStruct()));

		return bHasCharacterSkillRows && bHasSkillBalanceRows;
	}

	bool IsPassiveSkill(const FName SkillId)
	{
		// 현재 데이터 계약에는 능동/패시브 구분 필드가 없으므로, 쿨다운이 없는 것으로 확정된 패시브만 예외로 둔다.
		return SkillId == ProbePassiveSkillId;
	}

	const FScalableFloat* FindScalableFloat(
		const FGameplayEffectModifierMagnitude& Magnitude)
	{
		for (TFieldIterator<FStructProperty> PropertyIt(
			FGameplayEffectModifierMagnitude::StaticStruct()); PropertyIt; ++PropertyIt)
		{
			if (PropertyIt->Struct == FScalableFloat::StaticStruct())
			{
				return PropertyIt->ContainerPtrToValuePtr<FScalableFloat>(&Magnitude);
			}
		}

		return nullptr;
	}

	bool IsConstantScalableFloat(
		const FGameplayEffectModifierMagnitude& Magnitude,
		const bool bTreatZeroAsUnset)
	{
		if (Magnitude.GetMagnitudeCalculationType()
			!= EGameplayEffectMagnitudeCalculation::ScalableFloat)
		{
			return false;
		}

		const FScalableFloat* ScalableFloat = FindScalableFloat(Magnitude);
		if (!ScalableFloat || !ScalableFloat->IsStatic())
		{
			return false;
		}

		// HasDuration의 기본 0은 호출자가 SetDuration으로 채우기 위한 미설정 상태다.
		return !bTreatZeroAsUnset
			|| !FMath::IsNearlyZero(ScalableFloat->GetValueChecked());
	}

	int32 FindGameplayEffectComponentCount(const UGameplayEffect& Effect)
	{
		for (TFieldIterator<FArrayProperty> PropertyIt(
			UGameplayEffect::StaticClass(), EFieldIteratorFlags::IncludeSuper);
			PropertyIt;
			++PropertyIt)
		{
			const FObjectPropertyBase* InnerObjectProperty =
				CastField<FObjectPropertyBase>(PropertyIt->Inner);
			if (!InnerObjectProperty
				|| !InnerObjectProperty->PropertyClass->IsChildOf(
					UGameplayEffectComponent::StaticClass()))
			{
				continue;
			}

			void* ArrayAddress = PropertyIt->ContainerPtrToValuePtr<void>(
				const_cast<UGameplayEffect*>(&Effect));
			return FScriptArrayHelper(PropertyIt.operator->(), ArrayAddress).Num();
		}

		return INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSCooldownTagUniquenessTest,
	"LastFPS.Cooldown.Data.TagUniqueness",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSCooldownTagUniquenessTest::RunTest(const FString& Parameters)
{
	using namespace LastFPSCooldownAutomation;

	FGameDataContext DataContext;
	UDataTable* CharacterSkillTable = nullptr;
	UDataTable* SkillBalanceTable = nullptr;
	if (!LoadCooldownTables(
		*this, DataContext, CharacterSkillTable, SkillBalanceTable))
	{
		return false;
	}

	TArray<FLastFPSCharacterSkillData*> SkillRows;
	CharacterSkillTable->GetAllRows(GetTestFullName(), SkillRows);
	TMap<FName, TMap<FGameplayTag, FName>> SkillIdsByCharacterAndCooldownTag;

	for (const FLastFPSCharacterSkillData* SkillRow : SkillRows)
	{
		if (!SkillRow || IsPassiveSkill(SkillRow->SkillId))
		{
			continue;
		}

		if (!SkillRow->CooldownTag.IsValid())
		{
			AddError(FString::Printf(
				TEXT("캐릭터 '%s'의 스킬 '%s'에 유효한 CooldownTag가 없습니다."),
				*SkillRow->CharacterId.ToString(),
				*SkillRow->SkillId.ToString()));
			continue;
		}

		TMap<FGameplayTag, FName>& SkillIdsByCooldownTag =
			SkillIdsByCharacterAndCooldownTag.FindOrAdd(SkillRow->CharacterId);
		if (const FName* ExistingSkillId =
			SkillIdsByCooldownTag.Find(SkillRow->CooldownTag))
		{
			AddError(FString::Printf(
				TEXT("캐릭터 '%s'의 스킬 '%s'와 '%s'가 CooldownTag '%s'를 공유합니다."),
				*SkillRow->CharacterId.ToString(),
				*ExistingSkillId->ToString(),
				*SkillRow->SkillId.ToString(),
				*SkillRow->CooldownTag.ToString()));
			continue;
		}

		SkillIdsByCooldownTag.Add(SkillRow->CooldownTag, SkillRow->SkillId);
	}

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSCooldownBalanceReferenceCompletenessTest,
	"LastFPS.Cooldown.Data.BalanceReferenceCompleteness",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSCooldownBalanceReferenceCompletenessTest::RunTest(
	const FString& Parameters)
{
	using namespace LastFPSCooldownAutomation;

	FGameDataContext DataContext;
	UDataTable* CharacterSkillTable = nullptr;
	UDataTable* SkillBalanceTable = nullptr;
	if (!LoadCooldownTables(
		*this, DataContext, CharacterSkillTable, SkillBalanceTable))
	{
		return false;
	}

	TArray<FLastFPSCharacterSkillData*> SkillRows;
	CharacterSkillTable->GetAllRows(GetTestFullName(), SkillRows);
	for (const FLastFPSCharacterSkillData* SkillRow : SkillRows)
	{
		if (!SkillRow)
		{
			continue;
		}

		const FLastFPSSkillBalanceData* BalanceRow =
			SkillBalanceTable->FindRow<FLastFPSSkillBalanceData>(
				SkillRow->SkillId, GetTestFullName());
		if (!BalanceRow)
		{
			AddError(FString::Printf(
				TEXT("스킬 '%s'에 대응하는 밸런스 행이 없습니다."),
				*SkillRow->SkillId.ToString()));
			continue;
		}

		if (!IsPassiveSkill(SkillRow->SkillId)
			&& BalanceRow->Cooldown <= ActiveCooldownLowerBound)
		{
			AddError(FString::Printf(
				TEXT("능동 스킬 '%s'의 Cooldown은 0보다 커야 합니다. 현재 값: %g"),
				*SkillRow->SkillId.ToString(),
				BalanceRow->Cooldown));
		}
	}

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSGameplayEffectLiteralMagnitudeRegressionTest,
	"LastFPS.Cooldown.GameplayEffect.NoLiteralMagnitudes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSGameplayEffectLiteralMagnitudeRegressionTest::RunTest(
	const FString& Parameters)
{
	using namespace LastFPSCooldownAutomation;

	// 상태 효과 3종은 별도 상태 밸런스 계약이 필요해 이번 쿨다운 데이터화 범위에서 제외했다.
	// 즉시 회복 효과도 회복량 데이터 계약이 아직 없으므로 후속 데이터화 전까지 허용한다.
	const TSet<const UClass*> AllowedLiteralClasses = {
		ULastFPSGE_StatusSlow::StaticClass(),
		ULastFPSGE_StatusFreeze::StaticClass(),
		ULastFPSGE_MoveSpeedBuff::StaticClass(),
		ULastFPSGE_HealInstant::StaticClass()
	};

	const UPackage* LastFPSModulePackage =
		ULastFPSGE_Cooldown::StaticClass()->GetOutermost();
	TArray<UClass*> GameplayEffectClasses;
	GetDerivedClasses(
		UGameplayEffect::StaticClass(), GameplayEffectClasses, true);

	for (const UClass* GameplayEffectClass : GameplayEffectClasses)
	{
		if (!GameplayEffectClass
			|| GameplayEffectClass->GetOutermost() != LastFPSModulePackage
			|| GameplayEffectClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
			|| AllowedLiteralClasses.Contains(GameplayEffectClass))
		{
			continue;
		}

		const UGameplayEffect* Effect =
			GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
		if (!TestNotNull(
			*FString::Printf(TEXT("%s 클래스 기본 객체"), *GameplayEffectClass->GetName()),
			Effect))
		{
			continue;
		}

		if (Effect->DurationPolicy == EGameplayEffectDurationType::HasDuration
			&& IsConstantScalableFloat(Effect->DurationMagnitude, true))
		{
			AddError(FString::Printf(
				TEXT("GameplayEffect '%s'의 DurationMagnitude가 상수 ScalableFloat입니다."),
				*GameplayEffectClass->GetName()));
		}

		for (int32 ModifierIndex = 0;
			ModifierIndex < Effect->Modifiers.Num();
			++ModifierIndex)
		{
			if (IsConstantScalableFloat(
				Effect->Modifiers[ModifierIndex].ModifierMagnitude, false))
			{
				AddError(FString::Printf(
					TEXT("GameplayEffect '%s'의 Modifiers[%d].ModifierMagnitude가 상수 ScalableFloat입니다."),
					*GameplayEffectClass->GetName(),
					ModifierIndex));
			}
		}
	}

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLastFPSSharedCooldownClassInvariantTest,
	"LastFPS.Cooldown.GameplayEffect.SharedClassInvariant",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLastFPSSharedCooldownClassInvariantTest::RunTest(const FString& Parameters)
{
	using namespace LastFPSCooldownAutomation;

	const ULastFPSGE_Cooldown* CooldownEffect =
		GetDefault<ULastFPSGE_Cooldown>();
	if (!TestNotNull(TEXT("공유 쿨다운 클래스 기본 객체"), CooldownEffect))
	{
		return false;
	}

	TestEqual(
		TEXT("공유 쿨다운 클래스의 부여 태그 수"),
		CooldownEffect->GetGrantedTags().Num(),
		0);

	const int32 ComponentCount =
		FindGameplayEffectComponentCount(*CooldownEffect);
	if (ComponentCount == INDEX_NONE)
	{
		AddError(TEXT("UGameplayEffect의 GEComponents 배열을 찾지 못했습니다."));
	}
	else
	{
		TestEqual(
			TEXT("공유 쿨다운 클래스의 GEComponents 수"),
			ComponentCount,
			0);
	}

	return !HasAnyErrors();
}

#endif
