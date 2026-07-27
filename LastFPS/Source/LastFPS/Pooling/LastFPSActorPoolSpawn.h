#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"

namespace LastFPSActorPool
{
	/**
	 * 풀 인스턴스는 즉시 초기화하고, 풀 미구성 시에는 기존 Deferred Spawn 순서를 보존한다.
	 * 기능별 초기화가 BeginPlay 전에 필요했던 Actor와 재사용 Actor가 같은 호출 경로를 사용한다.
	 */
	template <typename TActor, typename TInitializer>
	TActor* AcquireOrSpawnDeferred(
		UWorld& World,
		const TSubclassOf<TActor> ActorClass,
		const FTransform& SpawnTransform,
		AActor* Owner,
		APawn* Instigator,
		TInitializer&& Initializer)
	{
		if (!ActorClass)
		{
			return nullptr;
		}

		if (ULastFPSActorPoolSubsystem* Pool =
			World.GetSubsystem<ULastFPSActorPoolSubsystem>())
		{
			if (TActor* PooledActor = Cast<TActor>(Pool->AcquireActorByClass(
				ActorClass,
				SpawnTransform,
				Owner,
				Instigator)))
			{
				Initializer(*PooledActor);
				return PooledActor;
			}
		}

		TActor* SpawnedActor = World.SpawnActorDeferred<TActor>(
			ActorClass,
			SpawnTransform,
			Owner,
			Instigator,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!SpawnedActor)
		{
			return nullptr;
		}

		Initializer(*SpawnedActor);
		SpawnedActor->FinishSpawning(SpawnTransform);
		return SpawnedActor;
	}
}
