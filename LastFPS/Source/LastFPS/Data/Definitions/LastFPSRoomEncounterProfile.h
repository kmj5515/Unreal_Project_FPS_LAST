#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSDestinationFeature.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSRoomEncounterProfile.generated.h"

class UDataTable;
class UMaterialInterface;
class UStaticMesh;

/** 배리어 충돌과 별개로 적용할 시각 표현 방식이다. */
UENUM(BlueprintType)
enum class ELastFPSRoomBarrierPresentationMode : uint8
{
	None,
	Mesh,
};

/** 방 출구를 막는 배리어의 모드별 시각 표현 설정이다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomBarrierPresentationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	ELastFPSRoomBarrierPresentationMode Mode = ELastFPSRoomBarrierPresentationMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FLinearColor Color = FLinearColor(1.0f, 0.015f, 0.01f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	float Opacity = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(ClampMin="0.1", UIMin="0.1", UIMax="12.0", EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	float FresnelPower = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual",
		meta=(ClampMin="0.0", UIMin="0.0", UIMax="20.0", EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	float FresnelIntensity = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	float DissolveDuration = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material Parameters",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FName ColorParameter = TEXT("BarrierColor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material Parameters",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FName OpacityParameter = TEXT("BarrierOpacity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material Parameters",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FName DissolveParameter = TEXT("DissolveProgress");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material Parameters",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FName FresnelPowerParameter = TEXT("FresnelPower");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material Parameters",
		meta=(EditCondition="Mode == ELastFPSRoomBarrierPresentationMode::Mesh", EditConditionHides))
	FName FresnelIntensityParameter = TEXT("FresnelIntensity");
};

/**
 * 하나의 GameMode가 사용하는 Room Encounter 데이터 계약이다.
 * 전역 설정을 사용하지 않고 모드별 테이블, 레벨 마커 계약, 표현과 성능 값을 함께 소유한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSRoomEncounterProfile : public ULastFPSDestinationFeature
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data",
		meta=(RowType="/Script/LastFPS.LastFPSRoomEncounterData"))
	TSoftObjectPtr<UDataTable> EncounterTable;

	/**
	 * 이 프로파일이 담당하는 목적지 식별자다.
	 * 테이블은 모든 목적지의 방을 함께 담고, 프로파일이 이 값으로 자기 몫만 걸러
	 * 프리로드와 런타임 생성 범위를 좁힌다. 비워 두면 모든 행을 사용한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
	FName DestinationId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Level Contract")
	FName TriggerMarkerTag = TEXT("RoomEncounter.Trigger");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Level Contract")
	FName BarrierMarkerTag = TEXT("RoomEncounter.Barrier");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Level Contract")
	FName SpawnMarkerTag = TEXT("RoomEncounter.Spawn");

	/** 목표 배치물(앵커)을 식별하는 Actor Tag 다. 정의의 ObjectiveTag 와 함께 붙인다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Level Contract")
	FName ObjectiveMarkerTag = TEXT("RoomEncounter.Objective");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Barrier Presentation")
	FLastFPSRoomBarrierPresentationSettings BarrierPresentation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Performance",
		meta=(ClampMin="1", UIMin="1", UIMax="8"))
	int32 MaxSpawnedActorsPerFrame = 1;

	virtual void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const override;

	bool IsConfigurationValid(FString& OutFailureReason) const;

	/** 이 프로파일이 담당하는 행인가. DestinationId 를 비워 둔 쪽은 항상 통과시킨다. */
	bool OwnsEncounterRow(FName RowDestinationId) const;
};
