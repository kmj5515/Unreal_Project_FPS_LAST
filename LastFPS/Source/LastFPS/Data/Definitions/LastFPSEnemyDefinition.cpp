#include "Data/Definitions/LastFPSEnemyDefinition.h"

#include "Data/Definitions/LastFPSWeaponDefinition.h"

void ULastFPSEnemyDefinition::GiveToAbilitySystem(UAbilitySystemComponent* ASC) const
{
	// 공통 스탯 적용
	Super::GiveToAbilitySystem(ASC);

	// TODO: 적 전용 스타트업 처리(예: AIProfile 기반 난이도 스케일링/버프)를 여기에 추가.
}

void ULastFPSEnemyDefinition::GatherSpawnDependencyPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	Super::GatherSpawnDependencyPaths(OutPaths);

	// AIProfile 은 PostInitializeComponents 에서, 초기 무기는 BeginPlay 에서 즉시 필요하다.
	const FSoftObjectPath Paths[] =
	{
		AIProfile.ToSoftObjectPath(),
		InitialWeaponDefinition.ToSoftObjectPath(),
	};

	for (const FSoftObjectPath& Path : Paths)
	{
		if (Path.IsValid())
		{
			OutPaths.AddUnique(Path);
		}
	}
}

void ULastFPSEnemyDefinition::GatherInitialWeaponDependencyPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	if (const ULastFPSWeaponDefinition* Weapon = InitialWeaponDefinition.Get())
	{
		Weapon->GatherEquipDependencyPaths(OutPaths);
	}
}
