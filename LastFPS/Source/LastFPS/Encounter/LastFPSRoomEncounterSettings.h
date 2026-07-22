#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "LastFPSRoomEncounterSettings.generated.h"

class UDataTable;
class UMaterialInterface;
class UStaticMesh;

/** 룸 출구를 막는 시각 배리어의 공통 표시 설정이다. */
USTRUCT(BlueprintType)
struct FLastFPSRoomBarrierPresentationSettings
{
	GENERATED_BODY()

	/** 충돌 박스 크기에 맞춰 늘려 사용할 기본 메시다. */
	UPROPERTY(Config, EditAnywhere, Category="Visual")
	TSoftObjectPtr<UStaticMesh> Mesh = TSoftObjectPtr<UStaticMesh>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));

	/** 반투명 홀로그램 머티리얼이다. DissolveProgress가 있으면 셰이더 소거도 함께 적용한다. */
	UPROPERTY(Config, EditAnywhere, Category="Visual")
	TSoftObjectPtr<UMaterialInterface> Material = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/FX/Materials/M_FX_RoomBarrierFresnel.M_FX_RoomBarrierFresnel")));

	UPROPERTY(Config, EditAnywhere, Category="Visual")
	FLinearColor Color = FLinearColor(1.0f, 0.015f, 0.01f, 1.0f);

	UPROPERTY(Config, EditAnywhere, Category="Visual", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Opacity = 0.18f;

	UPROPERTY(Config, EditAnywhere, Category="Visual", meta=(ClampMin="0.1", UIMin="0.1", UIMax="12.0"))
	float FresnelPower = 6.0f;

	UPROPERTY(Config, EditAnywhere, Category="Visual", meta=(ClampMin="0.0", UIMin="0.0", UIMax="20.0"))
	float FresnelIntensity = 8.0f;

	/** 길이 열릴 때 위에서 아래까지 완전히 지워지는 시간이다. */
	UPROPERTY(Config, EditAnywhere, Category="Animation", meta=(ClampMin="0.0", UIMin="0.0"))
	float DissolveDuration = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category="Material Parameters")
	FName ColorParameter = TEXT("BarrierColor");

	UPROPERTY(Config, EditAnywhere, Category="Material Parameters")
	FName OpacityParameter = TEXT("BarrierOpacity");

	UPROPERTY(Config, EditAnywhere, Category="Material Parameters")
	FName DissolveParameter = TEXT("DissolveProgress");

	UPROPERTY(Config, EditAnywhere, Category="Material Parameters")
	FName FresnelPowerParameter = TEXT("FresnelPower");

	UPROPERTY(Config, EditAnywhere, Category="Material Parameters")
	FName FresnelIntensityParameter = TEXT("FresnelIntensity");
};

/**
 * 방 전투의 공통 밸런스와 레벨 마커 계약을 제공한다.
 * 레벨은 Trigger/Barrier/Spawn 마커와 동일한 방 식별자 태그만 배치하면 된다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Room Encounter Settings"))
class LASTFPS_API ULastFPSRoomEncounterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Encounter 식별 태그와 Row Name을 연결하는 전투 구성 테이블이다. */
	UPROPERTY(Config, EditAnywhere, Category="Data", meta=(RowType="/Script/LastFPS.LastFPSRoomEncounterData"))
	TSoftObjectPtr<UDataTable> EncounterTable;

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName TriggerMarkerTag = TEXT("RoomEncounter.Trigger");

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName BarrierMarkerTag = TEXT("RoomEncounter.Barrier");

	UPROPERTY(Config, EditAnywhere, Category="Level Contract")
	FName SpawnMarkerTag = TEXT("RoomEncounter.Spawn");

	/** 모든 룸이 공유하는 출구 홀로그램 표시 설정이다. */
	UPROPERTY(Config, EditAnywhere, Category="Barrier Presentation")
	FLastFPSRoomBarrierPresentationSettings BarrierPresentation;
};
