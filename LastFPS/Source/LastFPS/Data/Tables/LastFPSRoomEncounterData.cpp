#include "Data/Tables/LastFPSRoomEncounterData.h"

bool FLastFPSRoomEncounterSpawnVFXDefinition::IsValid(FString& OutFailureReason) const
{
	if (DelayBeforeSpawn < 0.f)
	{
		OutFailureReason = TEXT("스폰 이펙트 이후 생성 지연 시간은 0보다 작을 수 없습니다.");
		return false;
	}

	if (Scale.X <= 0.f || Scale.Y <= 0.f || Scale.Z <= 0.f)
	{
		OutFailureReason = TEXT("생성 Niagara System의 Scale은 모든 축에서 0보다 커야 합니다.");
		return false;
	}

	return true;
}

bool FLastFPSRoomEncounterWaveDefinition::IsValid(FString& OutFailureReason) const
{
	if (DelayBeforeWave < 0.f)
	{
		OutFailureReason = TEXT("웨이브 시작 지연 시간이 0보다 작습니다.");
		return false;
	}

	if (SpawnInterval < 0.f)
	{
		OutFailureReason = TEXT("적 생성 간격이 0보다 작습니다.");
		return false;
	}

	if (SpawnBatchSize < 1)
	{
		OutFailureReason = TEXT("한 번에 생성할 적 수가 1보다 작습니다.");
		return false;
	}

	if (Units.IsEmpty())
	{
		OutFailureReason = TEXT("웨이브에 적 구성이 없습니다.");
		return false;
	}

	for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
	{
		const FLastFPSRoomEncounterUnitEntry& Unit = Units[UnitIndex];
		if (Unit.EnemyDefinition.IsNull())
		{
			OutFailureReason = FString::Printf(TEXT("적 구성 %d의 Character Definition이 비어 있습니다."), UnitIndex);
			return false;
		}

		if (Unit.Count < 1)
		{
			OutFailureReason = FString::Printf(TEXT("적 구성 %d의 수량이 1보다 작습니다."), UnitIndex);
			return false;
		}
	}

	return true;
}

bool FLastFPSRoomEncounterData::IsValid(FString& OutFailureReason) const
{
	if (Waves.IsEmpty())
	{
		OutFailureReason = TEXT("웨이브 목록이 비어 있습니다.");
		return false;
	}

	if (ReusedSpawnPointSpacing < 0.f)
	{
		OutFailureReason = TEXT("Spawn Point 재사용 간격이 0보다 작습니다.");
		return false;
	}

	if (SpawnPointRandomRadius < 0.f)
	{
		OutFailureReason = TEXT("Spawn Point 무작위 반경이 0보다 작습니다.");
		return false;
	}

	FString SpawnVFXFailureReason;
	if (!SpawnVFX.IsValid(SpawnVFXFailureReason))
	{
		OutFailureReason = FString::Printf(
			TEXT("생성 시각 효과 설정이 유효하지 않습니다: %s"),
			*SpawnVFXFailureReason);
		return false;
	}

	for (int32 WaveIndex = 0; WaveIndex < Waves.Num(); ++WaveIndex)
	{
		FString WaveFailureReason;
		if (!Waves[WaveIndex].IsValid(WaveFailureReason))
		{
			OutFailureReason = FString::Printf(
				TEXT("웨이브 %d 구성이 유효하지 않습니다: %s"),
				WaveIndex + 1,
				*WaveFailureReason);
			return false;
		}
	}

	return true;
}
