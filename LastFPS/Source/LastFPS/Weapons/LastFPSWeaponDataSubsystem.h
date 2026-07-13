#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSWeaponDataSubsystem.generated.h"

class UDataTable;
struct FLastFPSWeaponBalanceData;

/** 공용 무기 밸런스 테이블을 한 번 로드하고 WeaponId 기반 조회를 제공한다. */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSWeaponDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FLastFPSWeaponBalanceData* FindBalance(FName WeaponId) const;

protected:
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Weapons")
	TSoftObjectPtr<UDataTable> WeaponBalanceTable;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedWeaponBalanceTable;
};
