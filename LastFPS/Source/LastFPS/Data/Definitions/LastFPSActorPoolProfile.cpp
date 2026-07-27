#include "Data/Definitions/LastFPSActorPoolProfile.h"

void ULastFPSActorPoolProfile::CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	for (const FLastFPSActorPoolEntry& Entry : Pools)
	{
		if (!Entry.ActorClass.IsNull())
		{
			OutPaths.AddUnique(Entry.ActorClass.ToSoftObjectPath());
		}
	}
}

bool ULastFPSActorPoolProfile::IsConfigurationValid(FString& OutFailureReason) const
{
	TSet<FGameplayTag> PoolIds;
	TSet<FSoftObjectPath> ActorClasses;

	for (int32 Index = 0; Index < Pools.Num(); ++Index)
	{
		const FLastFPSActorPoolEntry& Entry = Pools[Index];
		if (!Entry.PoolId.IsValid())
		{
			OutFailureReason = FString::Printf(
				TEXT("Pools[%d]의 PoolId가 유효하지 않습니다."), Index);
			return false;
		}
		if (Entry.ActorClass.IsNull())
		{
			OutFailureReason = FString::Printf(
				TEXT("Pools[%d](%s)의 ActorClass가 비어 있습니다."),
				Index,
				*Entry.PoolId.ToString());
			return false;
		}
		if (Entry.InitialSize < 0 || Entry.MaxSize < 0)
		{
			OutFailureReason = FString::Printf(
				TEXT("Pools[%d](%s)의 크기는 음수일 수 없습니다."),
				Index,
				*Entry.PoolId.ToString());
			return false;
		}
		if (Entry.MaxSize > 0 && Entry.InitialSize > Entry.MaxSize)
		{
			OutFailureReason = FString::Printf(
				TEXT("Pools[%d](%s)의 InitialSize가 MaxSize보다 큽니다."),
				Index,
				*Entry.PoolId.ToString());
			return false;
		}
		if (Entry.InitialSize == 0 && Entry.MaxSize == 0)
		{
			OutFailureReason = FString::Printf(
				TEXT("Pools[%d](%s)의 InitialSize와 MaxSize가 모두 0입니다."),
				Index,
				*Entry.PoolId.ToString());
			return false;
		}
		if (PoolIds.Contains(Entry.PoolId))
		{
			OutFailureReason = FString::Printf(
				TEXT("PoolId가 중복되었습니다: %s"),
				*Entry.PoolId.ToString());
			return false;
		}

		const FSoftObjectPath ClassPath = Entry.ActorClass.ToSoftObjectPath();
		if (ActorClasses.Contains(ClassPath))
		{
			OutFailureReason = FString::Printf(
				TEXT("하나의 ActorClass를 여러 풀에 등록할 수 없습니다: %s"),
				*ClassPath.ToString());
			return false;
		}

		PoolIds.Add(Entry.PoolId);
		ActorClasses.Add(ClassPath);
	}

	OutFailureReason.Reset();
	return true;
}
