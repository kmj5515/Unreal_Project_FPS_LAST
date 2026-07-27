#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "LastFPSActorPoolSubsystem.generated.h"

class AActor;
class APawn;
class ULastFPSActorPoolProfile;
struct FLastFPSActorPoolEntry;

USTRUCT()
struct FLastFPSActorPoolBucket
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FGameplayTag PoolId;

	UPROPERTY(Transient)
	TObjectPtr<UClass> ActorClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> InactiveActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> ActiveActors;

	int32 MaxSize = 0;
	bool bRenderWarmup = true;
	bool bExhaustionReported = false;
};

/**
 * 월드 단위 Actor Pool 소유자다.
 * 구체 Actor 상태는 ILastFPSPoolableActor 계약에 위임하고 클래스별 저장소와 수명만 관리한다.
 */
UCLASS()
class LASTFPS_API ULastFPSActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	/**
	 * 중앙 비동기 로드가 끝난 뒤 호출한다.
	 * 동기 로드는 허용하지 않으며 이미 Resolve된 클래스만 초기 풀로 생성한다.
	 */
	bool PreparePools(
		const ULastFPSActorPoolProfile* Profile,
		const FTransform& WarmupTransform,
		TArray<AActor*>& OutRenderWarmupActors);

	/** 렌더 워밍업에 사용한 초기 인스턴스를 비활성 풀로 반환한다. */
	void FinalizePrewarm();

	UFUNCTION(BlueprintCallable, Category="LastFPS|Pool",
		meta=(DeterminesOutputType="ActorClass"))
	AActor* AcquireActorByClass(
		TSubclassOf<AActor> ActorClass,
		const FTransform& SpawnTransform,
		AActor* Owner = nullptr,
		APawn* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Pool")
	AActor* AcquireActor(
		FGameplayTag PoolId,
		const FTransform& SpawnTransform,
		AActor* Owner = nullptr,
		APawn* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category="LastFPS|Pool")
	bool ReleaseActor(AActor* Actor);

	UFUNCTION(BlueprintPure, Category="LastFPS|Pool")
	int32 GetAvailableCount(FGameplayTag PoolId) const;

	bool IsManagedActor(const AActor* Actor) const;

private:
	bool RegisterPoolEntry(
		const FLastFPSActorPoolEntry& Entry,
		const FTransform& WarmupTransform,
		TArray<AActor*>& OutRenderWarmupActors);
	AActor* CreateManagedActor(
		FLastFPSActorPoolBucket& Bucket,
		const FTransform& SpawnTransform);
	AActor* AcquireFromBucket(
		FLastFPSActorPoolBucket& Bucket,
		const FTransform& SpawnTransform,
		AActor* Owner,
		APawn* Instigator);
	void SuspendActor(AActor& Actor, bool bHideActor);
	void ActivateActor(
		AActor& Actor,
		const FTransform& SpawnTransform,
		AActor* Owner,
		APawn* Instigator);
	void RemoveInvalidActors(FLastFPSActorPoolBucket& Bucket);

	UPROPERTY(Transient)
	TMap<FGameplayTag, FLastFPSActorPoolBucket> Buckets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> PendingPrewarmActors;

	TMap<TObjectKey<UClass>, FGameplayTag> ClassToPoolId;
	TMap<TWeakObjectPtr<AActor>, FGameplayTag> ActorToPoolId;
};
