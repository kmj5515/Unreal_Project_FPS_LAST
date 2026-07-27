#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSStreamingLevelTransitionSettings.generated.h"

class AActor;
class ULastFPSEnemyDefinition;
class UWorld;

/** 동일한 Persistent World 안에서 미리 로드한 목적지 레벨로 이동하는 경로 설정이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSStreamingLevelTransitionRoute
{
	GENERATED_BODY()

	/** 로그와 네트워크상의 스트리밍 인스턴스 이름에 사용하는 안정적인 식별자다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	FName RouteId = NAME_None;

	/** 이 경로가 활성화되는 Persistent World다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	TSoftObjectPtr<UWorld> SourceWorld;

	/** 다른 TriggerBox와 구분하기 위한 역할 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger")
	FName TriggerMarkerTag = TEXT("RoomEncounter.Trigger");

	/** 전환 경로를 식별하는 TriggerBox의 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trigger")
	FName TriggerRouteTag = NAME_None;

	/** 숨김 상태로 미리 로드한 뒤 전환 시 표시할 목적지 레벨이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination")
	TSoftObjectPtr<UWorld> DestinationLevel;

	/**
	 * 목적지 PlayerStart를 고르는 선택 태그다.
	 * 비어 있으면 목적지 레벨에 PlayerStart가 정확히 하나일 때만 사용한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination")
	FName DestinationPlayerStartTag = NAME_None;

	/** 목적지 레벨 전체에 적용할 월드 변환이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination")
	FTransform DestinationLevelTransform = FTransform::Identity;

	/** PlayerStart 위치에 더할 최종 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination")
	FVector DestinationOffset = FVector::ZeroVector;

	/** 전환 완료 후 지연 생성할 적 정의다. 비어 있으면 적을 생성하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn")
	TSoftObjectPtr<ULastFPSEnemyDefinition> DelayedEnemyDefinition;

	/** 플레이어 전환이 끝난 시점부터 적을 생성할 때까지의 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn", meta=(ClampMin="0.0", Units="s"))
	float DelayedEnemySpawnDelay = 10.f;

	/**
	 * 목적지 TargetPoint를 고르는 Actor Tag다.
	 * 비어 있으면 목적지 레벨에 TargetPoint가 정확히 하나일 때만 그 지점을 사용한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn")
	FName DelayedEnemySpawnPointTag = NAME_None;

	/** 선택한 TargetPoint 위치에 더할 월드 공간 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn", meta=(Units="cm"))
	FVector DelayedEnemySpawnOffset = FVector::ZeroVector;

	/** 생성된 적의 ASC에서 한 번 실행할 스폰 Gameplay Cue 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn", meta=(Categories="GameplayCue"))
	FGameplayTag DelayedEnemySpawnGameplayCueTag;

	bool IsValid(FString& OutFailureReason) const;
};

/**
 * 맵 전환 경로의 프로젝트 설정이다.
 * 레벨과 트리거의 구체 조합은 런타임 시스템이 아닌 데이터에서 결정한다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LastFPS Streaming Level Transitions"))
class LASTFPS_API ULastFPSStreamingLevelTransitionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Transitions")
	TArray<FLastFPSStreamingLevelTransitionRoute> Routes;

	const FLastFPSStreamingLevelTransitionRoute* FindRouteForTrigger(
		const UWorld& World,
		const AActor& TriggerActor) const;

	void GetRoutesForWorld(
		const UWorld& World,
		TArray<const FLastFPSStreamingLevelTransitionRoute*>& OutRoutes) const;

private:
	static bool IsRouteForWorld(
		const FLastFPSStreamingLevelTransitionRoute& Route,
		const UWorld& World);
};
