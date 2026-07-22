#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSWeaponDataSubsystem.generated.h"

class UDataTable;
struct FLastFPSWeaponBalanceData;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponAmmoSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="1"))
	int32 MagazineCapacity = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0"))
	int32 StartingReserveAmmo = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0.01", Units="s"))
	float ReloadDuration = 2.f;
};

USTRUCT()
struct FLastFPSWeaponAmmoConfigEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Ammo")
	FName WeaponId;

	UPROPERTY(EditAnywhere, Category="Ammo")
	FLastFPSWeaponAmmoSettings Settings;
};

/** 공용 무기 밸런스 테이블을 한 번 로드하고 WeaponId 기반 조회를 제공한다. */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSWeaponDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FLastFPSWeaponBalanceData* FindBalance(FName WeaponId) const;
	const FLastFPSWeaponAmmoSettings* FindAmmoSettings(FName WeaponId) const;

protected:
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Weapons")
	TSoftObjectPtr<UDataTable> WeaponBalanceTable;

	/** 런타임 상태와 분리된 무기별 탄창 및 리로드 설정이다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Weapons")
	TArray<FLastFPSWeaponAmmoConfigEntry> WeaponAmmoConfigurations;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedWeaponBalanceTable;

	TMap<FName, FLastFPSWeaponAmmoSettings> AmmoSettingsByWeaponId;
};
