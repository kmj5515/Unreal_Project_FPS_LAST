#include "Encounter/LastFPSRoomEncounterSubsystem.h"

#include "Encounter/LastFPSRoomEncounterRuntime.h"
#include "Encounter/LastFPSRoomEncounterSettings.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Engine/DataTable.h"
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

	UDataTable* EncounterTable = Settings->EncounterTable.LoadSynchronous();
	if (!EncounterTable)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("방 전투 Data Table을 불러올 수 없습니다. Project Settings의 Room Encounter Settings를 확인하세요."));
		return;
	}

	if (EncounterTable->GetRowStruct() != FLastFPSRoomEncounterData::StaticStruct())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("방 전투 Data Table의 Row Structure가 FLastFPSRoomEncounterData가 아닙니다: %s"),
			*GetNameSafe(EncounterTable));
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

		const FName EncounterId = ResolveEncounterId(*TriggerVolume, *EncounterTable);
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
			EncounterTable->FindRow<FLastFPSRoomEncounterData>(EncounterId, RowContext, true);
		if (!EncounterData)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] Encounter Data Table 행을 찾을 수 없습니다: %s"),
				*EncounterId.ToString(),
				*GetNameSafe(EncounterTable));
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

		ATriggerBox* BarrierVolume = nullptr;
		for (AActor* Candidate : TriggerActors)
		{
			ATriggerBox* CandidateBox = Cast<ATriggerBox>(Candidate);
			if (CandidateBox
				&& CandidateBox->ActorHasTag(Settings->BarrierMarkerTag)
				&& CandidateBox->ActorHasTag(EncounterId))
			{
				BarrierVolume = CandidateBox;
				break;
			}
		}

		if (!BarrierVolume)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] 출구 차단 박스를 찾을 수 없습니다."),
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
			*BarrierVolume,
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
