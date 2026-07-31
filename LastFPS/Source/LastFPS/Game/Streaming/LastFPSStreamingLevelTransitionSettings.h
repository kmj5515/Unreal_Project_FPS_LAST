#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSStreamingLevelTransitionSettings.generated.h"

class AActor;
class ULastFPSEnemyDefinition;
class UMaterialInterface;
class UStaticMesh;
class UWorld;

/** 전환 도착 직후 플레이어를 둘러싸는 경고 원통 표현 설정이다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSArrivalContainmentPresentationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(EditCondition="bEnabled", EditConditionHides))
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(EditCondition="bEnabled", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> Material;

	/** 플레이어 액터 원점에서 원통 바닥까지의 보정값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout", meta=(EditCondition="bEnabled", EditConditionHides, Units="cm"))
	FVector Offset = FVector(0.f, 0.f, -90.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout", meta=(ClampMin="1.0", EditCondition="bEnabled", EditConditionHides, Units="cm"))
	float Radius = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout", meta=(ClampMin="1.0", EditCondition="bEnabled", EditConditionHides, Units="cm"))
	float Height = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warning Bands", meta=(ClampMin="1", ClampMax="16", EditCondition="bEnabled", EditConditionHides))
	int32 BandCount = 7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warning Bands", meta=(ClampMin="1.0", EditCondition="bEnabled", EditConditionHides, Units="cm"))
	float BandHeight = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(EditCondition="bEnabled", EditConditionHides))
	FLinearColor FieldColor = FLinearColor(1.f, 0.08f, 0.01f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnabled", EditConditionHides))
	float FieldOpacity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warning Bands", meta=(EditCondition="bEnabled", EditConditionHides))
	FLinearColor PrimaryBandColor = FLinearColor(1.f, 0.55f, 0.01f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warning Bands", meta=(EditCondition="bEnabled", EditConditionHides))
	FLinearColor SecondaryBandColor = FLinearColor(1.f, 0.03f, 0.01f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warning Bands", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnabled", EditConditionHides))
	float BandOpacity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(ClampMin="0.1", EditCondition="bEnabled", EditConditionHides))
	float FresnelPower = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(ClampMin="0.0", EditCondition="bEnabled", EditConditionHides))
	float FresnelIntensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(EditCondition="bEnabled", EditConditionHides))
	FName ColorParameter = TEXT("BarrierColor");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(EditCondition="bEnabled", EditConditionHides))
	FName OpacityParameter = TEXT("BarrierOpacity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(EditCondition="bEnabled", EditConditionHides))
	FName DissolveParameter = TEXT("DissolveProgress");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(EditCondition="bEnabled", EditConditionHides))
	FName FresnelPowerParameter = TEXT("FresnelPower");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material", meta=(EditCondition="bEnabled", EditConditionHides))
	FName FresnelIntensityParameter = TEXT("FresnelIntensity");

	bool IsValid(FString& OutFailureReason) const;
};

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

	/** 도착이 확정됐을 때 완료 처리할 위치 목표 태그다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Arrival", meta=(Categories="Location"))
	FGameplayTag ArrivalLocationTag;

	/** 도착 직후 플레이어 이동을 잠그는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Arrival", meta=(ClampMin="0.0", Units="s"))
	float ArrivalHoldDuration = 0.f;

	/** 이동 제한 중 표시할 경고 원통 표현이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Arrival")
	FLastFPSArrivalContainmentPresentationSettings ArrivalContainment;

	/** 전환 완료 후 지연 생성할 적 정의다. 비어 있으면 적을 생성하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn")
	TSoftObjectPtr<ULastFPSEnemyDefinition> DelayedEnemyDefinition;

	/** 지연 생성 적이 대표하는 Encounter ID다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delayed Enemy Spawn")
	FName DelayedEnemyEncounterId = NAME_None;

	/** 도착 이동 제한이 끝난 시점부터 적을 생성할 때까지의 시간이다. */
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
