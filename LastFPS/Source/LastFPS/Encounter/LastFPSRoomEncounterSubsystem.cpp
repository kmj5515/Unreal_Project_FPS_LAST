#include "Encounter/LastFPSRoomEncounterSubsystem.h"

#include "Data/Definitions/LastFPSDestinationContentSet.h"
#include "Data/Definitions/LastFPSRoomEncounterProfile.h"
#include "Data/Tables/LastFPSRoomEncounterData.h"
#include "Engine/DataTable.h"
#include "Engine/TargetPoint.h"
#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "Encounter/LastFPSRoomEncounterRuntime.h"
#include "Game/LastFPSGameModeBase.h"
#include "Game/Loading/LastFPSDestinationContentComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRoomEncounterSubsystem, Log, All);

namespace LastFPSRoomEncounterSubsystemInternal
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

	ALastFPSGameModeBase* GameMode =
		InWorld.GetAuthGameMode<ALastFPSGameModeBase>();
	ULastFPSDestinationContentSet* ContentSet =
		GameMode ? GameMode->GetDestinationContentSet() : nullptr;
	ActiveProfile = ContentSet
		? ContentSet->FindFeature<ULastFPSRoomEncounterProfile>()
		: nullptr;

	if (!ActiveProfile)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Verbose,
			TEXT("현재 GameMode에는 Room Encounter Profile이 없어 Encounter를 생성하지 않습니다: %s"),
			*GetNameSafe(GameMode));
		return;
	}

	FString FailureReason;
	if (!ActiveProfile->IsConfigurationValid(FailureReason))
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("Room Encounter Profile 구성이 유효하지 않습니다: Profile=%s, 원인=%s"),
			*GetNameSafe(ActiveProfile),
			*FailureReason);
		return;
	}

	AGameStateBase* GameState = InWorld.GetGameState();
	DestinationContentComponent = GameState
		? GameState->FindComponentByClass<ULastFPSDestinationContentComponent>()
		: nullptr;

	ULastFPSDestinationContentComponent* Content =
		DestinationContentComponent.Get();
	if (!Content)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("중앙 콘텐츠 로딩 컴포넌트가 없어 Encounter 초기화를 시작할 수 없습니다: GameState=%s"),
			*GetNameSafe(GameState));
		return;
	}

	if (Content->IsContentReady())
	{
		HandleDestinationContentReady();
		return;
	}

	ContentReadyHandle = Content->OnContentReady.AddUObject(
		this,
		&ThisClass::HandleDestinationContentReady);
}

void ULastFPSRoomEncounterSubsystem::HandleDestinationContentReady()
{
	if (bRuntimeEncountersInitialized)
	{
		return;
	}

	if (ULastFPSDestinationContentComponent* Content =
		DestinationContentComponent.Get())
	{
		Content->OnContentReady.Remove(ContentReadyHandle);
	}
	ContentReadyHandle.Reset();

	UWorld* World = GetWorld();
	if (!World || !ActiveProfile)
	{
		return;
	}

	UDataTable* EncounterTable = ActiveProfile->EncounterTable.Get();
	if (!EncounterTable)
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("중앙 콘텐츠 로딩이 완료됐지만 EncounterTable이 메모리에 없습니다: Profile=%s, Path=%s"),
			*GetNameSafe(ActiveProfile),
			*ActiveProfile->EncounterTable.ToString());
		return;
	}

	bRuntimeEncountersInitialized = true;
	InitializeRuntimeEncounters(*World, *EncounterTable, *ActiveProfile);
}

void ULastFPSRoomEncounterSubsystem::Deinitialize()
{
	if (ULastFPSDestinationContentComponent* Content =
		DestinationContentComponent.Get())
	{
		Content->OnContentReady.Remove(ContentReadyHandle);
	}

	ContentReadyHandle.Reset();
	DestinationContentComponent.Reset();
	RuntimeEncounters.Reset();
	ActiveProfile = nullptr;
	bRuntimeEncountersInitialized = false;
	Super::Deinitialize();
}

void ULastFPSRoomEncounterSubsystem::InitializeRuntimeEncounters(
	UWorld& InWorld,
	UDataTable& EncounterTable,
	const ULastFPSRoomEncounterProfile& Profile)
{
	if (EncounterTable.GetRowStruct()
		!= FLastFPSRoomEncounterData::StaticStruct())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Error,
			TEXT("Room Encounter Data Table의 Row Structure가 FLastFPSRoomEncounterData가 아닙니다: %s"),
			*GetNameSafe(&EncounterTable));
		return;
	}

	TArray<AActor*> TriggerActors;
	UGameplayStatics::GetAllActorsOfClass(
		&InWorld,
		ATriggerBox::StaticClass(),
		TriggerActors);

	TArray<AActor*> SpawnActors;
	UGameplayStatics::GetAllActorsOfClass(
		&InWorld,
		ATargetPoint::StaticClass(),
		SpawnActors);

	for (AActor* TriggerActor : TriggerActors)
	{
		ATriggerBox* TriggerVolume = Cast<ATriggerBox>(TriggerActor);
		if (!TriggerVolume
			|| !TriggerVolume->ActorHasTag(Profile.TriggerMarkerTag))
		{
			continue;
		}

		const FName EncounterId =
			LastFPSRoomEncounterSubsystemInternal::ResolveEncounterId(
				*TriggerVolume,
				EncounterTable);
		if (EncounterId.IsNone())
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("트리거 '%s'에 Data Table Row와 일치하는 Encounter 식별 태그가 없습니다."),
				*TriggerVolume->GetName());
			continue;
		}

		const FString RowContext = FString::Printf(
			TEXT("RoomEncounterSubsystem:%s"),
			*EncounterId.ToString());
		const FLastFPSRoomEncounterData* EncounterData =
			EncounterTable.FindRow<FLastFPSRoomEncounterData>(
				EncounterId,
				RowContext,
				true);
		if (!EncounterData)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] Encounter Data Table Row를 찾을 수 없습니다: %s"),
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
				&& CandidateBox->ActorHasTag(Profile.BarrierMarkerTag)
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
				&& SpawnPoint->ActorHasTag(Profile.SpawnMarkerTag)
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
			FName(*FString::Printf(
				TEXT("RoomEncounterRuntime_%s"),
				*EncounterId.ToString())));

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = RuntimeName;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ALastFPSRoomEncounterRuntime* Runtime =
			InWorld.SpawnActor<ALastFPSRoomEncounterRuntime>(
				ALastFPSRoomEncounterRuntime::StaticClass(),
				FTransform::Identity,
				SpawnParameters);
		if (!Runtime)
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Error,
				TEXT("[%s] Room Encounter Runtime Actor 생성에 실패했습니다."),
				*EncounterId.ToString());
			continue;
		}

		Runtime->InitializeEncounter(
			EncounterId,
			*TriggerVolume,
			BarrierVolumes,
			SpawnPoints,
			*EncounterData,
			Profile);
		RuntimeEncounters.Add(Runtime);
	}

	UE_LOG(
		LogLastFPSRoomEncounterSubsystem,
		Log,
		TEXT("Room Encounter Runtime %d개를 초기화했습니다: Profile=%s"),
		RuntimeEncounters.Num(),
		*GetNameSafe(&Profile));
}

#if !UE_BUILD_SHIPPING
bool ULastFPSRoomEncounterSubsystem::DebugClearEncounter(
	const FName EncounterId)
{
	UWorld* World = GetWorld();
	if (!World
		|| World->GetNetMode() == NM_Client
		|| EncounterId.IsNone())
	{
		UE_LOG(
			LogLastFPSRoomEncounterSubsystem,
			Warning,
			TEXT("Encounter 강제 완료 요청이 유효하지 않습니다: EncounterId=%s, World=%s"),
			*EncounterId.ToString(),
			*GetNameSafe(World));
		return false;
	}

	for (int32 RuntimeIndex = RuntimeEncounters.Num() - 1;
		RuntimeIndex >= 0;
		--RuntimeIndex)
	{
		ALastFPSRoomEncounterRuntime* Runtime =
			RuntimeEncounters[RuntimeIndex];
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
				TEXT("[%s] Encounter 강제 완료 결과: 성공"),
				*EncounterId.ToString());
		}
		else
		{
			UE_LOG(
				LogLastFPSRoomEncounterSubsystem,
				Warning,
				TEXT("[%s] Encounter 강제 완료 결과: 실패"),
				*EncounterId.ToString());
		}
		return bCompleted;
	}

	UE_LOG(
		LogLastFPSRoomEncounterSubsystem,
		Warning,
		TEXT("[%s] 강제 완료할 Room Encounter를 찾지 못했습니다."),
		*EncounterId.ToString());
	return false;
}
#endif

void ULastFPSRoomEncounterSubsystem::NotifyEncounterCleared(
	const FName EncounterId)
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

const UDataTable* ULastFPSRoomEncounterSubsystem::GetEncounterTable() const
{
	return ActiveProfile
		? ActiveProfile->EncounterTable.Get()
		: nullptr;
}
