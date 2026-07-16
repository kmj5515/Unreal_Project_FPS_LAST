#include "Skills/LastFPSSkillDataSubsystem.h"

#include "Data/Tables/LastFPSSkillBalanceData.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSSkillData, Log, All);

void ULastFPSSkillDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SkillsByCharacter.Reset();
	LoadedCharacterSkillTable = CharacterSkillTable.LoadSynchronous();
	LoadedSkillBalanceTable = SkillBalanceTable.LoadSynchronous();
	if (!LoadedCharacterSkillTable)
	{
		UE_LOG(LogLastFPSSkillData, Error,
			TEXT("공용 CharacterSkillTable을 로드하지 못했습니다. DefaultGame.ini의 LastFPSSkillDataSubsystem 설정을 확인하세요."));
		return;
	}
	if (!LoadedSkillBalanceTable)
	{
		UE_LOG(LogLastFPSSkillData, Error,
			TEXT("공용 SkillBalanceTable을 로드하지 못했습니다. DefaultGame.ini의 LastFPSSkillDataSubsystem 설정을 확인하세요."));
	}
	else if (!LoadedSkillBalanceTable->GetRowStruct()
		|| !LoadedSkillBalanceTable->GetRowStruct()->IsChildOf(FLastFPSSkillBalanceData::StaticStruct()))
	{
		UE_LOG(LogLastFPSSkillData, Error,
			TEXT("공용 SkillBalanceTable '%s'의 행 구조가 FLastFPSSkillBalanceData가 아닙니다."),
			*GetNameSafe(LoadedSkillBalanceTable));
		LoadedSkillBalanceTable = nullptr;
	}

	if (!LoadedCharacterSkillTable->GetRowStruct()
		|| !LoadedCharacterSkillTable->GetRowStruct()->IsChildOf(FLastFPSCharacterSkillData::StaticStruct()))
	{
		UE_LOG(LogLastFPSSkillData, Error,
			TEXT("공용 CharacterSkillTable '%s'의 행 구조가 FLastFPSCharacterSkillData가 아닙니다."),
			*GetNameSafe(LoadedCharacterSkillTable));
		LoadedCharacterSkillTable = nullptr;
		return;
	}

	TArray<FLastFPSCharacterSkillData*> Rows;
	LoadedCharacterSkillTable->GetAllRows(TEXT("LastFPSSkillDataSubsystem"), Rows);
	for (const FLastFPSCharacterSkillData* Row : Rows)
	{
		if (!Row || Row->CharacterId.IsNone() || Row->SkillId.IsNone())
		{
			UE_LOG(LogLastFPSSkillData, Error,
				TEXT("CharacterSkillTable '%s'에 CharacterId 또는 SkillId가 없는 행이 있습니다."),
				*GetNameSafe(LoadedCharacterSkillTable));
			continue;
		}
		if (!Row->InputTag.IsValid() || !Row->CooldownTag.IsValid())
		{
			UE_LOG(LogLastFPSSkillData, Error,
				TEXT("CharacterSkillTable '%s'의 SkillId '%s'에 입력 또는 쿨다운 태그가 없습니다."),
				*GetNameSafe(LoadedCharacterSkillTable), *Row->SkillId.ToString());
			continue;
		}
		if (LoadedSkillBalanceTable
			&& !LoadedSkillBalanceTable->FindRow<FLastFPSSkillBalanceData>(
				Row->SkillId, TEXT("LastFPSSkillDataSubsystem::Initialize"), false))
		{
			UE_LOG(LogLastFPSSkillData, Error,
				TEXT("SkillBalanceTable '%s'에 SkillId '%s'와 같은 이름의 행이 없습니다."),
				*GetNameSafe(LoadedSkillBalanceTable), *Row->SkillId.ToString());
		}

		TArray<const FLastFPSCharacterSkillData*>& CharacterSkills = SkillsByCharacter.FindOrAdd(Row->CharacterId);
		const bool bDuplicateSlot = CharacterSkills.ContainsByPredicate(
			[Row](const FLastFPSCharacterSkillData* ExistingRow)
			{
				return ExistingRow && ExistingRow->Slot == Row->Slot;
			});
		if (bDuplicateSlot)
		{
			UE_LOG(LogLastFPSSkillData, Error,
				TEXT("CharacterSkillTable '%s'에 CharacterId '%s'의 슬롯 %d가 중복되었습니다."),
				*GetNameSafe(LoadedCharacterSkillTable), *Row->CharacterId.ToString(), static_cast<int32>(Row->Slot));
			continue;
		}

		CharacterSkills.Add(Row);
	}

	for (const TPair<FName, TArray<const FLastFPSCharacterSkillData*>>& CharacterEntry : SkillsByCharacter)
	{
		if (CharacterEntry.Value.Num() != 4)
		{
			UE_LOG(LogLastFPSSkillData, Error,
				TEXT("CharacterId '%s'의 유효한 스킬 행은 4개여야 하지만 %d개입니다."),
				*CharacterEntry.Key.ToString(), CharacterEntry.Value.Num());
		}
	}
}

void ULastFPSSkillDataSubsystem::Deinitialize()
{
	SkillsByCharacter.Reset();
	LoadedCharacterSkillTable = nullptr;
	LoadedSkillBalanceTable = nullptr;
	Super::Deinitialize();
}

const FLastFPSCharacterSkillData* ULastFPSSkillDataSubsystem::FindSkill(
	const FName CharacterId,
	const ELastFPSCharacterSkillSlot Slot) const
{
	const TArray<const FLastFPSCharacterSkillData*>* CharacterSkills = SkillsByCharacter.Find(CharacterId);
	if (!CharacterSkills)
	{
		return nullptr;
	}

	const FLastFPSCharacterSkillData* const* Found = CharacterSkills->FindByPredicate(
		[Slot](const FLastFPSCharacterSkillData* Row)
		{
			return Row && Row->Slot == Slot;
		});
	return Found ? *Found : nullptr;
}

const FLastFPSCharacterSkillData* ULastFPSSkillDataSubsystem::FindSkillByInputTag(
	const FName CharacterId,
	const FGameplayTag InputTag) const
{
	if (!InputTag.IsValid())
	{
		return nullptr;
	}

	const TArray<const FLastFPSCharacterSkillData*>* CharacterSkills = SkillsByCharacter.Find(CharacterId);
	if (!CharacterSkills)
	{
		return nullptr;
	}

	const FLastFPSCharacterSkillData* const* Found = CharacterSkills->FindByPredicate(
		[InputTag](const FLastFPSCharacterSkillData* Row)
		{
			return Row && Row->InputTag.MatchesTagExact(InputTag);
		});
	return Found ? *Found : nullptr;
}

const FLastFPSCharacterSkillData* ULastFPSSkillDataSubsystem::FindSkillByAbilityTags(
	const FName CharacterId,
	const FGameplayTagContainer& AbilityTags) const
{
	if (AbilityTags.IsEmpty())
	{
		return nullptr;
	}

	const TArray<const FLastFPSCharacterSkillData*>* CharacterSkills = SkillsByCharacter.Find(CharacterId);
	if (!CharacterSkills)
	{
		return nullptr;
	}

	const FLastFPSCharacterSkillData* const* Found = CharacterSkills->FindByPredicate(
		[&AbilityTags](const FLastFPSCharacterSkillData* Row)
		{
			return Row && Row->InputTag.IsValid() && AbilityTags.HasTagExact(Row->InputTag);
		});
	return Found ? *Found : nullptr;
}

const FLastFPSSkillBalanceData* ULastFPSSkillDataSubsystem::FindBalance(const FName SkillId) const
{
	if (!LoadedSkillBalanceTable || SkillId.IsNone())
	{
		return nullptr;
	}

	return LoadedSkillBalanceTable->FindRow<FLastFPSSkillBalanceData>(
		SkillId,
		TEXT("LastFPSSkillDataSubsystem::FindBalance"),
		false);
}

