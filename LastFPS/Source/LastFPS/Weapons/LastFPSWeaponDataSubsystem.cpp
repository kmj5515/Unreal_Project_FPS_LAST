#include "Weapons/LastFPSWeaponDataSubsystem.h"

#include "Data/Tables/LastFPSWeaponBalanceData.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSWeaponData, Log, All);

void ULastFPSWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadedWeaponBalanceTable = WeaponBalanceTable.LoadSynchronous();
	if (!LoadedWeaponBalanceTable)
	{
		UE_LOG(LogLastFPSWeaponData, Error,
			TEXT("공용 WeaponBalanceTable을 로드하지 못했습니다. DefaultGame.ini의 LastFPSWeaponDataSubsystem 설정을 확인하세요."));
		return;
	}

	if (!LoadedWeaponBalanceTable->GetRowStruct()
		|| !LoadedWeaponBalanceTable->GetRowStruct()->IsChildOf(FLastFPSWeaponBalanceData::StaticStruct()))
	{
		UE_LOG(LogLastFPSWeaponData, Error,
			TEXT("공용 WeaponBalanceTable '%s'의 행 구조가 FLastFPSWeaponBalanceData가 아닙니다."),
			*GetNameSafe(LoadedWeaponBalanceTable));
		LoadedWeaponBalanceTable = nullptr;
		return;
	}

	TArray<FLastFPSWeaponBalanceData*> Rows;
	LoadedWeaponBalanceTable->GetAllRows(TEXT("LastFPSWeaponDataSubsystem::Initialize"), Rows);
	if (Rows.IsEmpty())
	{
		UE_LOG(LogLastFPSWeaponData, Warning,
			TEXT("공용 WeaponBalanceTable '%s'에 무기 행이 없습니다."),
			*GetNameSafe(LoadedWeaponBalanceTable));
	}
}

void ULastFPSWeaponDataSubsystem::Deinitialize()
{
	LoadedWeaponBalanceTable = nullptr;
	Super::Deinitialize();
}

const FLastFPSWeaponBalanceData* ULastFPSWeaponDataSubsystem::FindBalance(const FName WeaponId) const
{
	if (!LoadedWeaponBalanceTable || WeaponId.IsNone())
	{
		return nullptr;
	}

	return LoadedWeaponBalanceTable->FindRow<FLastFPSWeaponBalanceData>(
		WeaponId,
		TEXT("LastFPSWeaponDataSubsystem::FindBalance"),
		false);
}
