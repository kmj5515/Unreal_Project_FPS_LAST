#include "Data/Definitions/LastFPSRoomEncounterProfile.h"

#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Engine/DataTable.h"

void ULastFPSRoomEncounterProfile::CollectRequiredPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	const FSoftObjectPath EncounterTablePath = EncounterTable.ToSoftObjectPath();
	if (EncounterTablePath.IsValid())
	{
		OutPaths.AddUnique(EncounterTablePath);
	}

	if (BarrierPresentation.Mode == ELastFPSRoomBarrierPresentationMode::Mesh)
	{
		const FSoftObjectPath MeshPath =
			BarrierPresentation.Mesh.ToSoftObjectPath();
		if (MeshPath.IsValid())
		{
			OutPaths.AddUnique(MeshPath);
		}

		const FSoftObjectPath MaterialPath =
			BarrierPresentation.Material.ToSoftObjectPath();
		if (MaterialPath.IsValid())
		{
			OutPaths.AddUnique(MaterialPath);
		}
	}

	const UDataTable* LoadedEncounterTable = EncounterTable.Get();
	if (!LoadedEncounterTable
		|| LoadedEncounterTable->GetRowStruct()
			!= FLastFPSRoomEncounterData::StaticStruct())
	{
		return;
	}

	TArray<FLastFPSRoomEncounterData*> EncounterRows;
	LoadedEncounterTable->GetAllRows(
		TEXT("LastFPSRoomEncounterProfile"),
		EncounterRows);
	for (const FLastFPSRoomEncounterData* EncounterRow : EncounterRows)
	{
		if (!EncounterRow)
		{
			continue;
		}

		const FSoftObjectPath SpawnVFXPath =
			EncounterRow->SpawnVFX.NiagaraSystem.ToSoftObjectPath();
		if (SpawnVFXPath.IsValid())
		{
			OutPaths.AddUnique(SpawnVFXPath);
		}

		for (const FLastFPSRoomEncounterWaveDefinition& Wave
			: EncounterRow->Waves)
		{
			for (const FLastFPSRoomEncounterUnitEntry& Unit : Wave.Units)
			{
				const FSoftObjectPath DefinitionPath =
					Unit.EnemyDefinition.ToSoftObjectPath();
				if (DefinitionPath.IsValid())
				{
					OutPaths.AddUnique(DefinitionPath);
				}
			}
		}
	}
}

bool ULastFPSRoomEncounterProfile::IsConfigurationValid(
	FString& OutFailureReason) const
{
	if (EncounterTable.IsNull())
	{
		OutFailureReason = TEXT("EncounterTable이 지정되지 않았습니다.");
		return false;
	}

	if (TriggerMarkerTag.IsNone()
		|| BarrierMarkerTag.IsNone()
		|| SpawnMarkerTag.IsNone())
	{
		OutFailureReason = TEXT("Trigger, Barrier 또는 Spawn 마커 태그가 비어 있습니다.");
		return false;
	}

	if (MaxSpawnedActorsPerFrame < 1)
	{
		OutFailureReason = TEXT("MaxSpawnedActorsPerFrame은 1 이상이어야 합니다.");
		return false;
	}

	if (BarrierPresentation.Mode == ELastFPSRoomBarrierPresentationMode::Mesh
		&& (BarrierPresentation.Mesh.IsNull()
			|| BarrierPresentation.Material.IsNull()))
	{
		OutFailureReason = TEXT("Mesh 배리어 표현에는 Mesh와 Material이 모두 필요합니다.");
		return false;
	}

	OutFailureReason.Reset();
	return true;
}
