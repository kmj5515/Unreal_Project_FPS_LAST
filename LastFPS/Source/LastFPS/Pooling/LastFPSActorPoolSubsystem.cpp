#include "Pooling/LastFPSActorPoolSubsystem.h"

#include "Data/Definitions/LastFPSActorPoolProfile.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Pooling/LastFPSPoolableActor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSActorPool, Log, All);

bool ULastFPSActorPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World
		&& (World->WorldType == EWorldType::Game
			|| World->WorldType == EWorldType::PIE
			|| World->WorldType == EWorldType::GamePreview);
}

void ULastFPSActorPoolSubsystem::Deinitialize()
{
	PendingPrewarmActors.Reset();
	ActorToPoolId.Reset();
	ClassToPoolId.Reset();
	Buckets.Reset();
	Super::Deinitialize();
}

bool ULastFPSActorPoolSubsystem::PreparePools(
	const ULastFPSActorPoolProfile* Profile,
	const FTransform& WarmupTransform,
	TArray<AActor*>& OutRenderWarmupActors)
{
	OutRenderWarmupActors.Reset();
	PendingPrewarmActors.Reset();

	if (!Profile)
	{
		return true;
	}

	FString FailureReason;
	if (!Profile->IsConfigurationValid(FailureReason))
	{
		UE_LOG(
			LogLastFPSActorPool,
			Error,
			TEXT("Actor Pool Profile 구성이 유효하지 않습니다: Profile=%s, 원인=%s"),
			*GetNameSafe(Profile),
			*FailureReason);
		return false;
	}

	bool bAllEntriesPrepared = true;
	for (const FLastFPSActorPoolEntry& Entry : Profile->Pools)
	{
		bAllEntriesPrepared &=
			RegisterPoolEntry(Entry, WarmupTransform, OutRenderWarmupActors);
	}

	UE_LOG(
		LogLastFPSActorPool,
		Log,
		TEXT("Actor Pool 준비 완료: Profile=%s, Pool=%d개, 렌더 워밍업 Actor=%d개"),
		*GetNameSafe(Profile),
		Buckets.Num(),
		OutRenderWarmupActors.Num());
	return bAllEntriesPrepared;
}

bool ULastFPSActorPoolSubsystem::RegisterPoolEntry(
	const FLastFPSActorPoolEntry& Entry,
	const FTransform& WarmupTransform,
	TArray<AActor*>& OutRenderWarmupActors)
{
	UClass* LoadedClass = Entry.ActorClass.Get();
	if (!LoadedClass)
	{
		UE_LOG(
			LogLastFPSActorPool,
			Error,
			TEXT("중앙 로드 완료 후에도 풀 ActorClass가 메모리에 없습니다: PoolId=%s, Path=%s"),
			*Entry.PoolId.ToString(),
			*Entry.ActorClass.ToString());
		return false;
	}
	if (!LoadedClass->IsChildOf(AActor::StaticClass())
		|| !LoadedClass->ImplementsInterface(ULastFPSPoolableActor::StaticClass()))
	{
		UE_LOG(
			LogLastFPSActorPool,
			Error,
			TEXT("풀 클래스는 AActor와 ILastFPSPoolableActor를 구현해야 합니다: PoolId=%s, Class=%s"),
			*Entry.PoolId.ToString(),
			*GetNameSafe(LoadedClass));
		return false;
	}

	FLastFPSActorPoolBucket& Bucket = Buckets.FindOrAdd(Entry.PoolId);
	Bucket.PoolId = Entry.PoolId;
	Bucket.ActorClass = LoadedClass;
	Bucket.MaxSize = FMath::Max(Entry.MaxSize, Entry.InitialSize);
	Bucket.bRenderWarmup = Entry.bRenderWarmup;
	ClassToPoolId.Add(TObjectKey<UClass>(LoadedClass), Entry.PoolId);

	const int32 RequiredCount =
		FMath::Max(0, Entry.InitialSize - Bucket.InactiveActors.Num() - Bucket.ActiveActors.Num());
	for (int32 Index = 0; Index < RequiredCount; ++Index)
	{
		AActor* Actor = CreateManagedActor(Bucket, WarmupTransform);
		if (!Actor)
		{
			return false;
		}

		ILastFPSPoolableActor::Execute_OnReleasedToPool(Actor);
		SuspendActor(*Actor, true);

		if (Entry.bRenderWarmup)
		{
			ILastFPSPoolableActor::Execute_OnPrepareForPoolRenderWarmup(Actor);
			PendingPrewarmActors.Add(Actor);
			OutRenderWarmupActors.Add(Actor);
		}
		else
		{
			Bucket.ActiveActors.Remove(Actor);
			Bucket.InactiveActors.Add(Actor);
		}
	}

	return true;
}

AActor* ULastFPSActorPoolSubsystem::CreateManagedActor(
	FLastFPSActorPoolBucket& Bucket,
	const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World || !Bucket.ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	AActor* Actor = World->SpawnActor<AActor>(
		Bucket.ActorClass,
		SpawnTransform,
		SpawnParameters);
	if (!Actor)
	{
		UE_LOG(
			LogLastFPSActorPool,
			Error,
			TEXT("풀 Actor 생성에 실패했습니다: PoolId=%s, Class=%s"),
			*Bucket.PoolId.ToString(),
			*GetNameSafe(Bucket.ActorClass));
		return nullptr;
	}

	// 로딩 워밍업 인스턴스가 원격 클라이언트에 실제 게임 Actor처럼 보이지 않게 한다.
	// 첫 Acquire에서 클래스 기본 복제 정책을 복구한다.
	if (Actor->GetIsReplicated())
	{
		Actor->SetReplicates(false);
	}

	Bucket.ActiveActors.Add(Actor);
	ActorToPoolId.Add(TWeakObjectPtr<AActor>(Actor), Bucket.PoolId);
	return Actor;
}

void ULastFPSActorPoolSubsystem::FinalizePrewarm()
{
	TArray<TObjectPtr<AActor>> ActorsToRelease = MoveTemp(PendingPrewarmActors);
	PendingPrewarmActors.Reset();

	for (AActor* Actor : ActorsToRelease)
	{
		ReleaseActor(Actor);
	}
}

AActor* ULastFPSActorPoolSubsystem::AcquireActorByClass(
	const TSubclassOf<AActor> ActorClass,
	const FTransform& SpawnTransform,
	AActor* Owner,
	APawn* Instigator)
{
	UClass* ResolvedClass = ActorClass.Get();
	if (!ResolvedClass)
	{
		return nullptr;
	}

	const FGameplayTag* PoolId = ClassToPoolId.Find(TObjectKey<UClass>(ResolvedClass));
	return PoolId
		? AcquireActor(*PoolId, SpawnTransform, Owner, Instigator)
		: nullptr;
}

AActor* ULastFPSActorPoolSubsystem::AcquireActor(
	const FGameplayTag PoolId,
	const FTransform& SpawnTransform,
	AActor* Owner,
	APawn* Instigator)
{
	FLastFPSActorPoolBucket* Bucket = Buckets.Find(PoolId);
	if (!Bucket)
	{
		return nullptr;
	}
	return AcquireFromBucket(*Bucket, SpawnTransform, Owner, Instigator);
}

AActor* ULastFPSActorPoolSubsystem::AcquireFromBucket(
	FLastFPSActorPoolBucket& Bucket,
	const FTransform& SpawnTransform,
	AActor* Owner,
	APawn* Instigator)
{
	RemoveInvalidActors(Bucket);

	AActor* Actor = nullptr;
	while (!Bucket.InactiveActors.IsEmpty() && !Actor)
	{
		Actor = Bucket.InactiveActors.Pop(EAllowShrinking::No);
		if (!IsValid(Actor))
		{
			Actor = nullptr;
		}
	}

	const int32 CurrentSize = Bucket.InactiveActors.Num() + Bucket.ActiveActors.Num();
	if (!Actor && CurrentSize < Bucket.MaxSize)
	{
		Actor = CreateManagedActor(Bucket, SpawnTransform);
	}
	else if (Actor)
	{
		Bucket.ActiveActors.Add(Actor);
	}

	if (!Actor)
	{
		if (!Bucket.bExhaustionReported)
		{
			Bucket.bExhaustionReported = true;
			UE_LOG(
				LogLastFPSActorPool,
				Warning,
				TEXT("Actor Pool이 소진되어 기존 Spawn 경로를 사용합니다: PoolId=%s, MaxSize=%d"),
				*Bucket.PoolId.ToString(),
				Bucket.MaxSize);
		}
		return nullptr;
	}

	Bucket.bExhaustionReported = false;
	ActivateActor(*Actor, SpawnTransform, Owner, Instigator);
	ILastFPSPoolableActor::Execute_OnAcquiredFromPool(Actor);
	return Actor;
}

bool ULastFPSActorPoolSubsystem::ReleaseActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const FGameplayTag* PoolId =
		ActorToPoolId.Find(TWeakObjectPtr<AActor>(Actor));
	FLastFPSActorPoolBucket* Bucket = PoolId ? Buckets.Find(*PoolId) : nullptr;
	if (!Bucket)
	{
		return false;
	}

	if (Bucket->InactiveActors.Contains(Actor))
	{
		return true;
	}

	ILastFPSPoolableActor::Execute_OnReleasedToPool(Actor);
	SuspendActor(*Actor, true);
	Bucket->ActiveActors.Remove(Actor);
	Bucket->InactiveActors.Add(Actor);
	Bucket->bExhaustionReported = false;
	return true;
}

void ULastFPSActorPoolSubsystem::SuspendActor(
	AActor& Actor,
	const bool bHideActor)
{
	Actor.SetLifeSpan(0.f);
	if (UWorld* World = Actor.GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(&Actor);
	}

	if (APawn* Pawn = Cast<APawn>(&Actor))
	{
		if (AController* Controller = Pawn->GetController())
		{
			Controller->StopMovement();
			Controller->UnPossess();
			Controller->Destroy();
		}
	}

	Actor.SetActorEnableCollision(false);
	Actor.SetActorTickEnabled(false);
	Actor.SetActorHiddenInGame(bHideActor);
	Actor.SetOwner(nullptr);
	Actor.SetInstigator(nullptr);
	Actor.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Actor.ForceNetUpdate();
}

void ULastFPSActorPoolSubsystem::ActivateActor(
	AActor& Actor,
	const FTransform& SpawnTransform,
	AActor* Owner,
	APawn* Instigator)
{
	const AActor* ClassDefaultActor =
		Actor.GetClass()->GetDefaultObject<AActor>();
	if (ClassDefaultActor && ClassDefaultActor->GetIsReplicated())
	{
		Actor.SetReplicates(true);
	}
	Actor.SetNetDormancy(DORM_Awake);
	Actor.FlushNetDormancy();
	Actor.SetActorTransform(
		SpawnTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Actor.SetOwner(Owner);
	Actor.SetInstigator(Instigator);
	Actor.SetActorHiddenInGame(false);
	Actor.ForceNetUpdate();
}

void ULastFPSActorPoolSubsystem::RemoveInvalidActors(
	FLastFPSActorPoolBucket& Bucket)
{
	Bucket.InactiveActors.RemoveAll(
		[](const TObjectPtr<AActor>& Actor)
		{
			return !IsValid(Actor);
		});
	Bucket.ActiveActors.RemoveAll(
		[](const TObjectPtr<AActor>& Actor)
		{
			return !IsValid(Actor);
		});
}

int32 ULastFPSActorPoolSubsystem::GetAvailableCount(const FGameplayTag PoolId) const
{
	const FLastFPSActorPoolBucket* Bucket = Buckets.Find(PoolId);
	return Bucket ? Bucket->InactiveActors.Num() : 0;
}

bool ULastFPSActorPoolSubsystem::IsManagedActor(const AActor* Actor) const
{
	return IsValid(Actor)
		&& ActorToPoolId.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(Actor)));
}
