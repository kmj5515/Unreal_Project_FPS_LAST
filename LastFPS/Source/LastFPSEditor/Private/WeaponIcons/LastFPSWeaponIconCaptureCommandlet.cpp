#include "WeaponIcons/LastFPSWeaponIconCaptureCommandlet.h"

#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "WeaponIcons/LastFPSWeaponIconCaptureActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSWeaponIconCaptureCommandlet, Log, All);

ULastFPSWeaponIconCaptureCommandlet::ULastFPSWeaponIconCaptureCommandlet()
{
	IsClient = true;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 ULastFPSWeaponIconCaptureCommandlet::Main(const FString& Params)
{
	if (!IsAllowCommandletRendering())
	{
		UE_LOG(
			LogLastFPSWeaponIconCaptureCommandlet,
			Error,
			TEXT("무기 슬롯 아이콘 촬영에는 -AllowCommandletRendering 옵션이 필요합니다."));
		return 1;
	}

	FString ItemTablePath = TEXT("/Game/Data/Tables/Hub/DT_ItemData.DT_ItemData");
	FParse::Value(*Params, TEXT("ItemTable="), ItemTablePath);
	UDataTable* ItemTable = LoadObject<UDataTable>(nullptr, *ItemTablePath);
	if (!ItemTable)
	{
		UE_LOG(
			LogLastFPSWeaponIconCaptureCommandlet,
			Error,
			TEXT("아이템 테이블을 불러오지 못했습니다: %s"),
			*ItemTablePath);
		return 2;
	}

	FString PngOutputDirectory = FPaths::ProjectSavedDir() / TEXT("WeaponSlotIcons");
	FParse::Value(*Params, TEXT("PngOutputDirectory="), PngOutputDirectory);
	PngOutputDirectory = FPaths::ConvertRelativePathToFull(PngOutputDirectory);

	UWorld::InitializationValues InitializationValues;
	InitializationValues
		.AllowAudioPlayback(false)
		.CreatePhysicsScene(false)
		.CreateFXSystem(false)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false);

	UWorld* World = UWorld::CreateWorld(
		EWorldType::Editor,
		true,
		TEXT("LastFPSWeaponIconCaptureWorld"),
		nullptr,
		true,
		ERHIFeatureLevel::Num,
		&InitializationValues);
	if (!World)
	{
		UE_LOG(LogLastFPSWeaponIconCaptureCommandlet, Error, TEXT("무기 슬롯 아이콘 촬영 월드를 만들지 못했습니다."));
		return 3;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Editor);
	WorldContext.SetCurrentWorld(World);
	World->UpdateWorldComponents(true, false);

	ALastFPSWeaponIconCaptureActor* CaptureActor = World->SpawnActor<ALastFPSWeaponIconCaptureActor>();
	if (!CaptureActor)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(true);
		UE_LOG(LogLastFPSWeaponIconCaptureCommandlet, Error, TEXT("무기 슬롯 아이콘 촬영 액터를 만들지 못했습니다."));
		return 4;
	}

	CaptureActor->ConfigureBatchCapture(ItemTable, PngOutputDirectory);
	World->UpdateWorldComponents(true, false);
	CaptureActor->CaptureAllWeaponIcons();
	const int32 CapturedCount = CaptureActor->GetLastCapturedCount();

	const bool bSavedPackages = UEditorLoadingAndSavingUtils::SaveDirtyPackages(false, true);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(true);

	if (CapturedCount <= 0 || !bSavedPackages)
	{
		UE_LOG(
			LogLastFPSWeaponIconCaptureCommandlet,
			Error,
			TEXT("무기 슬롯 아이콘 생성 실패: 생성=%d, 패키지저장=%s"),
			CapturedCount,
			bSavedPackages ? TEXT("성공") : TEXT("실패"));
		return 5;
	}

	UE_LOG(
		LogLastFPSWeaponIconCaptureCommandlet,
		Display,
		TEXT("무기 슬롯 아이콘 생성 완료: %d개, PNG=%s"),
		CapturedCount,
		*PngOutputDirectory);
	return 0;
}
