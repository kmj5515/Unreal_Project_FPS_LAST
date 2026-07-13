#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSCharacterSkillData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSSkillDataSubsystem.generated.h"

class UDataTable;
struct FLastFPSSkillBalanceData;

/** 공용 캐릭터 스킬 테이블을 한 번 로드하고 캐릭터별 조회를 제공한다. */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSSkillDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FLastFPSCharacterSkillData* FindSkill(
		FName CharacterId,
		ELastFPSCharacterSkillSlot Slot) const;

	const FLastFPSCharacterSkillData* FindSkillByInputTag(
		FName CharacterId,
		FGameplayTag InputTag) const;

	const FLastFPSCharacterSkillData* FindSkillByAbilityTags(
		FName CharacterId,
		const FGameplayTagContainer& AbilityTags) const;

	const FLastFPSSkillBalanceData* FindBalance(FName SkillId) const;

protected:
	/** 모든 캐릭터가 공유하는 FLastFPSCharacterSkillData 테이블이다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Skills")
	TSoftObjectPtr<UDataTable> CharacterSkillTable;

	/** SkillId와 같은 행 이름을 사용하는 FLastFPSSkillBalanceData 테이블이다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Skills")
	TSoftObjectPtr<UDataTable> SkillBalanceTable;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedCharacterSkillTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedSkillBalanceTable;

	TMap<FName, TArray<const FLastFPSCharacterSkillData*>> SkillsByCharacter;
};
