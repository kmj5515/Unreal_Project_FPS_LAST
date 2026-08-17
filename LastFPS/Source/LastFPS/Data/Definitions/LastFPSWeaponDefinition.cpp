#include "Data/Definitions/LastFPSWeaponDefinition.h"

#include "Data/AssetManagement/LastFPSPrimaryAssetTypes.h"

const FPrimaryAssetType ULastFPSWeaponDefinition::PrimaryAssetType =
	LastFPSPrimaryAssetTypes::WeaponDefinition;

FPrimaryAssetId ULastFPSWeaponDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

namespace
{
	// UWeaponComponent::ApplyWeaponDefinitionValues 가 장착 시점에 동기 해석하는 참조들이다.
	// 그쪽 목록이 바뀌면 여기도 함께 바꿔야 프리로드가 계속 유효하다.
	template <typename FuncType>
	void ForEachEquipDependency(const ULastFPSWeaponDefinition& Definition, FuncType&& Func)
	{
		Func(Definition.SkeletalMesh.ToSoftObjectPath());
		Func(Definition.WeaponActorClass.ToSoftObjectPath());
		Func(Definition.ProjectileClass.ToSoftObjectPath());
		Func(Definition.AnimLayerClass.ToSoftObjectPath());
		Func(Definition.FireAnimation.ToSoftObjectPath());
		Func(Definition.FireSound.ToSoftObjectPath());
		Func(Definition.MuzzleFlashEffect.ToSoftObjectPath());
		Func(Definition.FireCameraShakeClass.ToSoftObjectPath());
	}
}

void ULastFPSWeaponDefinition::GatherEquipDependencyPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	ForEachEquipDependency(
		*this,
		[&OutPaths](const FSoftObjectPath& Path)
		{
			if (Path.IsValid())
			{
				OutPaths.AddUnique(Path);
			}
		});
}

bool ULastFPSWeaponDefinition::AreEquipDependenciesResident() const
{
	bool bResident = true;
	ForEachEquipDependency(
		*this,
		[&bResident](const FSoftObjectPath& Path)
		{
			// 경로가 비어 있으면 로드할 것이 없으므로 상주로 본다.
			bResident &= !Path.IsValid() || Path.ResolveObject() != nullptr;
		});
	return bResident;
}
