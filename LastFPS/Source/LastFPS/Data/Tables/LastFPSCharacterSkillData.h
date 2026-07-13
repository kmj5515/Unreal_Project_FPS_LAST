#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LastFPSCharacterSkillData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ELastFPSCharacterSkillSlot : uint8
{
	Skill1,
	Skill2,
	Skill3,
	Ultimate
};

/** 캐릭터 스킬의 표시 정보와 데이터 연결만 소유하며 Ability 클래스는 포함하지 않는다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSCharacterSkillData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill")
	FName SkillId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill")
	ELastFPSCharacterSkillSlot Slot = ELastFPSCharacterSkillSlot::Skill1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|UI")
	FText KeyLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|GAS")
	FGameplayTag InputTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|GAS")
	FGameplayTag CooldownTag;
};
