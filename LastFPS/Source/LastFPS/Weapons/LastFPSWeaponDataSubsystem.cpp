#include "Weapons/LastFPSWeaponDataSubsystem.h"

#include "Data/Tables/LastFPSWeaponBalanceData.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSWeaponData, Log, All);

void ULastFPSWeaponDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AmmoSettingsByWeaponId.Reset();
	for (const FLastFPSWeaponAmmoConfigEntry& Entry : WeaponAmmoConfigurations)
	{
		if (Entry.WeaponId.IsNone()
			|| Entry.Settings.MagazineCapacity <= 0
			|| Entry.Settings.StartingReserveAmmo < 0
			|| Entry.Settings.ReloadDuration <= 0.f)
		{
			UE_LOG(LogLastFPSWeaponData, Error,
				TEXT("무기 탄약 설정이 올바르지 않습니다. WeaponId='%s', MagazineCapacity=%d, StartingReserveAmmo=%d, ReloadDuration=%.2f"),
				*Entry.WeaponId.ToString(),
				Entry.Settings.MagazineCapacity,
				Entry.Settings.StartingReserveAmmo,
				Entry.Settings.ReloadDuration);
			continue;
		}

		if (AmmoSettingsByWeaponId.Contains(Entry.WeaponId))
		{
			UE_LOG(LogLastFPSWeaponData, Warning,
				TEXT("무기 '%s'의 탄약 설정이 중복되어 마지막 설정을 사용합니다."),
				*Entry.WeaponId.ToString());
		}

		AmmoSettingsByWeaponId.Add(Entry.WeaponId, Entry.Settings);
	}

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
	AmmoSettingsByWeaponId.Reset();
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

const FLastFPSWeaponAmmoSettings* ULastFPSWeaponDataSubsystem::FindAmmoSettings(const FName WeaponId) const
{
	if (const FLastFPSWeaponBalanceData* Balance = FindBalance(WeaponId))
	{
		static thread_local FLastFPSWeaponAmmoSettings TempSettings;
		TempSettings.MagazineCapacity    = Balance->MagazineCapacity;
		TempSettings.StartingReserveAmmo = Balance->StartingReserveAmmo;
		TempSettings.ReloadDuration      = Balance->ReloadDuration;
		return &TempSettings;
	}

	return WeaponId.IsNone() ? nullptr : AmmoSettingsByWeaponId.Find(WeaponId);
}
