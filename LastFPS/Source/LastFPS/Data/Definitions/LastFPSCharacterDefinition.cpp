#include "Data/Definitions/LastFPSCharacterDefinition.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"
#include "Data/Characters/LastFPSCharacterStatData.h"

const FPrimaryAssetType ULastFPSCharacterDefinition::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::CharacterDefinition;

FPrimaryAssetId ULastFPSCharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

bool ULastFPSCharacterDefinition::HasClassificationTag(const FGameplayTag TagToCheck) const
{
	return TagToCheck.IsValid() && ClassificationTags.HasTag(TagToCheck);
}

void ULastFPSCharacterDefinition::GiveToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}

	// 공통: 캐릭터 기본 스탯(AttributeSet 초기값)을 ASC 에 적용.
	// StatData 는 소프트 참조라 정의 애셋만 로드된 시점에는 아직 메모리에 없다.
	// Get() 기반 검사(if (StatData))로는 조용히 건너뛰어 AttributeSet 의 C++ 기본값이 그대로 남으므로
	// 반드시 로드한 결과로 판단한다. 스탯은 스폰 즉시 필요해 동기 로드한다.
	if (StatData.IsNull())
	{
		return;
	}

	if (const ULastFPSCharacterStatData* LoadedStatData = StatData.LoadSynchronous())
	{
		LoadedStatData->ApplyToAbilitySystem(ASC);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("%s: StatData '%s' 로드에 실패해 기본 스탯을 적용하지 못했습니다."),
			*GetName(),
			*StatData.ToString());
	}

	// 어빌리티/스타트업 이펙트 부여는 서버 권한이 필요해
	// ALastFPSCharacterBase::GiveDefaultAbilities / ApplyDefaultEffects 에서 처리한다.
}

void ULastFPSCharacterDefinition::GatherSpawnDependencyPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	// 스폰 직후 동기 로드되는 참조들이다.
	// PawnClass: 스폰 자체, StatData: GiveToAbilitySystem,
	// VisualData/AbilitySet: ALastFPSCharacterBase 초기화 경로.
	const FSoftObjectPath Paths[] =
	{
		PawnClass.ToSoftObjectPath(),
		StatData.ToSoftObjectPath(),
		VisualData.ToSoftObjectPath(),
		AbilitySet.ToSoftObjectPath(),
	};

	for (const FSoftObjectPath& Path : Paths)
	{
		if (Path.IsValid())
		{
			OutPaths.AddUnique(Path);
		}
	}
}
