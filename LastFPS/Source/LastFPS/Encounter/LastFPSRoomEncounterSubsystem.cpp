#include "Encounter/LastFPSRoomEncounterSubsystem.h"

#include "Encounter/LastFPSRoomEncounterRuntime.h"
#include "Encounter/LastFPSRoomEncounterSettings.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/StreamableManager.h"
#include "Engine/TargetPoint.h"
#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRoomEncounterSubsystem, Log, All);

namespace
{
	FName ResolveEncounterId(
		const AActor& Actor,
		const UDataTable& EncounterTable)
	{
		for (const FName& Tag : Actor.Tags)
		{
			if (EncounterTable.GetRowMap().Contains(Tag))
			{
				return Tag;
			}
		}
		return NAME_None;
	}
}

bool ULastFPSRoomEncounterSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World
		&& (World->WorldType == EWorldType::Game
			|| World->WorldType == EWorldType::PIE
			|| World->WorldType == EWorldType::GamePreview);
}

void ULastFPSRoomEncounterSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}

	const ULastFPSRoomEncounterSettings* Settings = GetDefault<ULastFPSRoomEncounterSettings>();
	if (!Settings)
	{
		UE_LOG(LogLastFPSRoomEncounterSubsystem, Error, TEXT("방 전투 설정을 찾을 수 없습니다."));
		return;
	}

	UDataTable* EncounterTable = Settings->EncounterTable.Get();
	if (EncounterTable)
	{
		InitializeRuntimeEncounters(InWorld, *EncounterTable);
		return;
	}

	const FSoftObjectPath EncounterTablePath = Settings->EncounterTable.ToSoftObjectPath();
	if (!EncounterTablePath.IsValid())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("Room Encounter Data Table 경로가 설정되지 않았습니다."));
		return;
	}

	EncounterTableLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		EncounterTablePath,
		FStreamableDelegate::CreateUObject(
			this,
			&ULastFPSRoomEncounterSubsystem::HandleEncounterTableLoaded),
		FStreamableManager::AsyncLoadHighPriority);
	if (!EncounterTableLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("Room Encounter Data Table 비동기 로드를 시작하지 못했습니다: %s"),
			*EncounterTablePath.ToString());
	}
}

void ULastFPSRoomEncounterSubsystem::HandleEncounterTableLoaded()
{
	UWorld* World = GetWorld();
	const ULastFPSRoomEncounterSettings* Settings = GetDefault<ULastFPSRoomEncounterSettings>();
	UDataTable* EncounterTable = Settings ? Settings->EncounterTable.Get() : nullptr;
	if (!EncounterTable)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("Room Encounter Data Table 비동기 로드 결과가 유효하지 않습니다."));
		EncounterTableLoadHandle.Reset();
		return;
	}

	if (!World)
	{
		EncounterTableLoadHandle.Reset();
		return;
	}

	InitializeRuntimeEncounters(*World, *EncounterTable);
	EncounterTableLoadHandle.Reset();
}

void ULastFPSRoomEncounterSubsystem::Deinitialize()
{
	if (EncounterTableLoadHandle.IsValid())
	{
		EncounterTableLoadHandle->CancelHandle();
		EncounterTableLoadHandle.Reset();
	}
	RuntimeEncounters.Reset();
	Super::Deinitialize();
}

void ULastFPSRoomEncounterSubsystem::InitializeRuntimeEncounters(
	UWorld& InWorld,
	UDataTable& EncounterTable)
{
	const ULastFPSRoomEncounterSettings* Settings = GetDefault<ULastFPSRoomEncounterSettings>();
	if (!Settings)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("룸 인카운터 런타임 생성 실패: 설정 기본 객체를 찾을 수 없습니다."));
		return;
	}

	if (EncounterTable.GetRowStruct() != FLastFPSRoomEncounterData::StaticStruct())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("방 전투 Data Table의 Row Structure가 FLastFPSRoomEncounterData가 아닙니다: %s"),
			*GetNameSafe(&EncounterTable));
		return;
	}

	TArray<AActor*> TriggerActors;
	UGameplayStatics::GetAllActorsOfClass(&InWorld, ATriggerBox::StaticClass(), TriggerActors);

	TArray<AActor*> SpawnActors;
	UGameplayStatics::GetAllActorsOfClass(&InWorld, ATargetPoint::StaticClass(), SpawnActors);

	for (AActor* TriggerActor : TriggerActors)
	{
		ATriggerBox* TriggerVolume = Cast<ATriggerBox>(TriggerActor);
		if (!TriggerVolume || !TriggerVolume->ActorHasTag(Settings->TriggerMarkerTag))
		{
			continue;
		}

		const FName EncounterId = ResolveEncounterId(*TriggerVolume, EncounterTable);
		if (EncounterId.IsNone())
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("트리거 %s에 Data Table 행과 일치하는 Encounter 식별 태그가 없습니다."),
				*TriggerVolume->GetName());
			continue;
		}

		const FString RowContext = FString::Printf(
			TEXT("RoomEncounterSubsystem:%s"),
			*EncounterId.ToString());
		const FLastFPSRoomEncounterData* EncounterData =
			EncounterTable.FindRow<FLastFPSRoomEncounterData>(EncounterId, RowContext, true);
		if (!EncounterData)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] Encounter Data Table 행을 찾을 수 없습니다: %s"),
				*EncounterId.ToString(),
				*GetNameSafe(&EncounterTable));
			continue;
		}

		FString EncounterFailureReason;
		if (!EncounterData->IsValid(EncounterFailureReason))
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] Encounter 데이터가 유효하지 않습니다: %s"),
				*EncounterId.ToString(),
				*EncounterFailureReason);
			continue;
		}

		TArray<ATriggerBox*> BarrierVolumes;
		for (AActor* Candidate : TriggerActors)
		{
			ATriggerBox* CandidateBox = Cast<ATriggerBox>(Candidate);
			if (CandidateBox
				&& CandidateBox->ActorHasTag(Settings->BarrierMarkerTag)
				&& CandidateBox->ActorHasTag(EncounterId))
			{
				BarrierVolumes.Add(CandidateBox);
			}
		}

		if (BarrierVolumes.IsEmpty())
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] 배리어 볼륨을 하나도 찾을 수 없습니다."),
				*EncounterId.ToString());
			continue;
		}

		TArray<ATargetPoint*> SpawnPoints;
		for (AActor* SpawnActor : SpawnActors)
		{
			ATargetPoint* SpawnPoint = Cast<ATargetPoint>(SpawnActor);
			if (SpawnPoint
				&& SpawnPoint->ActorHasTag(Settings->SpawnMarkerTag)
				&& SpawnPoint->ActorHasTag(EncounterId))
			{
				SpawnPoints.Add(SpawnPoint);
			}
		}

		if (SpawnPoints.IsEmpty())
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] 적 스폰 지점을 찾을 수 없습니다."),
				*EncounterId.ToString());
			continue;
		}

		const FName RuntimeName = MakeUniqueObjectName(
			&InWorld,
			ALastFPSRoomEncounterRuntime::StaticClass(),
			FName(*FString::Printf(TEXT("RoomEncounterRuntime_%s"), *EncounterId.ToString())));
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = RuntimeName;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ALastFPSRoomEncounterRuntime* Runtime = InWorld.SpawnActor<ALastFPSRoomEncounterRuntime>(
			ALastFPSRoomEncounterRuntime::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
		if (!Runtime)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] 런타임 전투 액터 생성에 실패했습니다."),
				*EncounterId.ToString());
			continue;
		}

		Runtime->InitializeEncounter(
			EncounterId,
			*TriggerVolume,
			BarrierVolumes,
			SpawnPoints,
			*EncounterData);
		RuntimeEncounters.Add(Runtime);
	}

	UE_LOG(
		LogLastFPSRoomEncounterSubsystem,
		Log,
		TEXT("방 전투 런타임 %d개를 초기화했습니다."),
		RuntimeEncounters.Num());
}

#if !UE_BUILD_SHIPPING
bool ULastFPSRoomEncounterSubsystem::DebugClearEncounter(const FName EncounterId)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || EncounterId.IsNone())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Warning,
			TEXT("인카운터 클리어 치트 요청이 유효하지 않습니다. EncounterId=%s, World=%s"),
			*EncounterId.ToString(),
			*GetNameSafe(World));
		return false;
	}

	for (int32 RuntimeIndex = RuntimeEncounters.Num() - 1; RuntimeIndex >= 0; --RuntimeIndex)
	{
		ALastFPSRoomEncounterRuntime* Runtime = RuntimeEncounters[RuntimeIndex];
		if (!IsValid(Runtime))
		{
			RuntimeEncounters.RemoveAtSwap(RuntimeIndex);
			continue;
		}

		if (Runtime->GetEncounterIdForDebug() != EncounterId)
		{
			continue;
		}

		const bool bCompleted = Runtime->DebugForceCompleteEncounter();
		if (bCompleted)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Display,
				TEXT("[%s] 인카운터 클리어 치트 완료"),
				*EncounterId.ToString());
		}
		else
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Warning,
				TEXT("[%s] 인카운터 클리어 치트 실패: 해당 구역에 진입해 인카운터가 시작됐는지 확인하세요."),
				*EncounterId.ToString());
		}
		return bCompleted;
	}

	UE_LOG(
		LogLastFPSRoomEncounterSubsystem,
		Warning,
		TEXT("[%s] 클리어할 런타임 인카운터를 찾지 못했습니다."),
		*EncounterId.ToString());
	return false;
}
#endif

void ULastFPSRoomEncounterSubsystem::NotifyEncounterCleared(FName EncounterId)
{
	if (OnEncounterCleared.IsBound())
	{
		OnEncounterCleared.Broadcast(EncounterId);
	}
}

void ULastFPSRoomEncounterSubsystem::NotifyEncounterProgress(
	const FName EncounterId,
	const int32 DefeatedEnemyCount,
	const int32 TotalEnemyCount)
{
	if (EncounterId.IsNone() || TotalEnemyCount < 1)
	{
		return;
	}

	OnEncounterProgressChanged.Broadcast(
		EncounterId,
		FMath::Clamp(DefeatedEnemyCount, 0, TotalEnemyCount),
		TotalEnemyCount);
}
