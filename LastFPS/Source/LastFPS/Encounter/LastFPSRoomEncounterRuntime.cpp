#include "Encounter/LastFPSRoomEncounterRuntime.h"

#include "Algo/AllOf.h"
#include "Character/LastFPSCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Data/Definitions/LastFPSEncounterObjectiveDefinition.h"
#include "Encounter/LastFPSTimedObjectiveComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "Encounter/LastFPSRoomBarrierPresentationComponent.h"
#include "Encounter/LastFPSRoomSpawnPresentationComponent.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSRoomEncounter, Log, All);

ALastFPSRoomEncounterRuntime::ALastFPSRoomEncounterRuntime()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SpawnPresentationComponent = CreateDefaultSubobject<ULastFPSRoomSpawnPresentationComponent>(
		TEXT("SpawnPresentationComponent"));
}

void ALastFPSRoomEncounterRuntime::InitializeEncounter(
	const FName InEncounterId,
	ATriggerBox& InTriggerVolume,
	const TArray<ATriggerBox*>& InBarrierVolumes,
	const TArray<ATargetPoint*>& InSpawnPoints,
	const FLastFPSRoomEncounterData& InEncounterData,
	const ULastFPSRoomEncounterProfile& InProfile)
{
	if (!HasAuthority() || bInitialized)
	{
		return;
	}

	EncounterId = InEncounterId;
	TriggerVolume = &InTriggerVolume;
	BarrierVolumes.Reserve(InBarrierVolumes.Num());
	for (ATriggerBox* BarrierVolume : InBarrierVolumes)
	{
		if (IsValid(BarrierVolume))
		{
			BarrierVolumes.AddUnique(BarrierVolume);
		}
	}
	if (BarrierVolumes.IsEmpty())
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 유효한 배리어 볼륨이 없어 인카운터를 초기화할 수 없습니다."),
			*EncounterId.ToString());
		return;
	}

	Waves = InEncounterData.Waves;
	EnemyDefinitionAssets.Reset();
	for (const FLastFPSRoomEncounterWaveDefinition& Wave : Waves)
	{
		for (const FLastFPSRoomEncounterUnitEntry& Unit : Wave.Units)
		{
			if (!Unit.EnemyDefinition.IsNull())
			{
				EnemyDefinitionAssets.AddUnique(Unit.EnemyDefinition);
			}
		}
	}
	BaseTotalEnemyCount = InEncounterData.GetTotalEnemyCount();
	TotalEnemyCount = BaseTotalEnemyCount;
	DefeatedEnemyCount = 0;
	ReusedSpawnPointSpacing = FMath::Max(InEncounterData.ReusedSpawnPointSpacing, 0.f);
	SpawnPointRandomRadius = FMath::Max(InEncounterData.SpawnPointRandomRadius, 0.f);
	SpawnDelayAfterVFX = InEncounterData.SpawnVFX.NiagaraSystem.IsNull()
		? 0.f
		: FMath::Max(InEncounterData.SpawnVFX.DelayBeforeSpawn, 0.f);
	BarrierPresentationSettings = InProfile.BarrierPresentation;
	MaxSpawnedActorsPerFrame = FMath::Max(InProfile.MaxSpawnedActorsPerFrame, 1);
	SpawnPresentationComponent->Configure(InEncounterData.SpawnVFX);
	ConfigureBarrierPresentation();

	SpawnPoints.Reserve(InSpawnPoints.Num());
	for (ATargetPoint* SpawnPoint : InSpawnPoints)
	{
		if (IsValid(SpawnPoint))
		{
			SpawnPoints.Add(SpawnPoint);
		}
	}

	// 목표 생성은 전투 시작까지 미룬다 — 배치물이 스트리밍 서브레벨에 있으면
	// 지금은 월드에 존재하지 않는다. 여기서는 무엇을 만들지만 기억해 둔다.
	ObjectiveEntries = InEncounterData.Objectives;
	ObjectiveMarkerTag = InProfile.ObjectiveMarkerTag;

	TriggerVolume->OnActorBeginOverlap.AddUniqueDynamic(
		this,
		&ALastFPSRoomEncounterRuntime::HandleTriggerOverlap);
	SetBarrierActive(false);
	bInitialized = true;
	BeginEnemyDefinitionPreload();

	// Data Table 비동기 로드 중 이미 진입한 플레이어도 시작 요청을 잃지 않게 한다.
	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		const APawn* OverlappingPawn = Cast<APawn>(OverlappingActor);
		if (OverlappingPawn && OverlappingPawn->IsPlayerControlled())
		{
			StartEncounter();
			break;
		}
	}

	BroadcastEncounterProgress();
	ForceNetUpdate();
}

void ALastFPSRoomEncounterRuntime::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, BarrierVolumes);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, bBarrierActive);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, BarrierPresentationSettings);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, bEncounterCleared);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, CurrentWave);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, WaveLoopCount);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, EncounterId);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, TotalEnemyCount);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, DefeatedEnemyCount);
	DOREPLIFETIME(ALastFPSRoomEncounterRuntime, EnemyDefinitionAssets);
}

void ALastFPSRoomEncounterRuntime::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);

	if (IsValid(TriggerVolume))
	{
		TriggerVolume->OnActorBeginOverlap.RemoveDynamic(
			this,
			&ALastFPSRoomEncounterRuntime::HandleTriggerOverlap);
	}

	for (const TWeakObjectPtr<ALastFPSCharacterBase>& EnemyPtr : AliveEnemies)
	{
		if (ALastFPSCharacterBase* Enemy = EnemyPtr.Get())
		{
			Enemy->OnDeath.RemoveAll(this);
			Enemy->OnDestroyed.RemoveDynamic(
				this,
				&ALastFPSRoomEncounterRuntime::HandleEnemyDestroyed);
		}
	}
	AliveEnemies.Reset();
	StopObjectives();
	for (ULastFPSTimedObjectiveComponent* Objective : Objectives)
	{
		if (!IsValid(Objective))
		{
			continue;
		}

		Objective->OnObjectiveResolved.RemoveDynamic(
			this,
			&ALastFPSRoomEncounterRuntime::HandleObjectiveResolved);

		// 런타임이 만들어 붙인 컴포넌트이므로 런타임이 걷어낸다.
		// 남겨 두면 앵커 액터에 비활성 컴포넌트가 누적된다.
		Objective->DestroyComponent();
	}
	Objectives.Reset();
	CancelEnemyDefinitionPreload();
	CancelBarrierPresentationLoad();
	LoadedEnemyDefinitions.Reset();
	PendingEnemySpawns.Reset();
	ResetBarrierPresentations();

	Super::EndPlay(EndPlayReason);
}

void ALastFPSRoomEncounterRuntime::HandleTriggerOverlap(
	AActor* /*OverlappedActor*/,
	AActor* OtherActor)
{
	if (!HasAuthority() || bEncounterStarted || bEncounterCleared)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	StartEncounter();
}

void ALastFPSRoomEncounterRuntime::StartEncounter()
{
	if (!HasAuthority() || !bInitialized || bEncounterStarted)
	{
		return;
	}

	if (!bEnemyDefinitionsReady)
	{
		bStartRequested = true;
		if (bEnemyDefinitionsFailed)
		{
			bStartRequested = false;
			AbortEncounterOnConfigurationError(TEXT("적 Character Definition 비동기 로드가 실패했습니다."));
		}
		else
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Verbose,
				TEXT("[%s] 적 에셋 비동기 로드가 끝날 때까지 인카운터 시작을 보류합니다."),
				*EncounterId.ToString());
		}
		return;
	}

	bStartRequested = false;
	bEncounterStarted = true;
	if (IsValid(TriggerVolume))
	{
		TriggerVolume->SetActorEnableCollision(false);
	}

	if (SpawnPoints.IsEmpty())
	{
		AbortEncounterOnConfigurationError(TEXT("Spawn Point가 없습니다."));
		return;
	}

	if (Waves.IsEmpty())
	{
		AbortEncounterOnConfigurationError(TEXT("웨이브 데이터가 없습니다."));
		return;
	}

	SetBarrierActive(true);
	CreateObjectives();
	StartObjectives();
	ScheduleNextWave();
}

void ALastFPSRoomEncounterRuntime::CreateObjectives()
{
	if (bObjectivesCreated || ObjectiveEntries.IsEmpty())
	{
		return;
	}
	bObjectivesCreated = true;

	UWorld* World = GetWorld();
	if (!World || ObjectiveMarkerTag.IsNone())
	{
		return;
	}

	// 플레이어가 방에 들어온 시점이라 스트리밍 서브레벨도 이미 로드돼 있다.
	TArray<AActor*> ObjectiveAnchors;
	UGameplayStatics::GetAllActorsWithTag(World, ObjectiveMarkerTag, ObjectiveAnchors);

	for (const FLastFPSEncounterObjectiveEntry& Entry : ObjectiveEntries)
	{
		const ULastFPSEncounterObjectiveDefinition* Definition = Entry.Definition.Get();
		if (!Definition)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 목표 정의가 로드되지 않았습니다: %s"),
				*EncounterId.ToString(),
				*Entry.Definition.ToString());
			continue;
		}

		FString FailureReason;
		if (!Definition->IsConfigurationValid(FailureReason))
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 목표 정의 구성이 유효하지 않습니다: 정의=%s, 원인=%s"),
				*EncounterId.ToString(),
				*GetNameSafe(Definition),
				*FailureReason);
			continue;
		}

		// 같은 마커 태그를 단 다른 방의 배치물과 섞이지 않도록 인카운터 소속까지 함께 본다.
		AActor* const* FoundAnchor = ObjectiveAnchors.FindByPredicate(
			[this, Definition](const AActor* Candidate)
			{
				return IsValid(Candidate)
					&& Candidate->ActorHasTag(EncounterId)
					&& Candidate->ActorHasTag(Definition->ObjectiveTag);
			});
		if (!FoundAnchor)
		{
			// 배치 누락을 조용히 넘기면 방이 그냥 섬멸형으로 동작해 원인 추적이 어려워진다.
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 목표 배치물을 찾지 못했습니다: 정의=%s, ObjectiveTag=%s"),
				*EncounterId.ToString(),
				*GetNameSafe(Definition),
				*Definition->ObjectiveTag.ToString());
			continue;
		}

		// 목표 컴포넌트는 앵커의 서브오브젝트로 복제되므로, 앵커가 복제되지 않으면
		// 클라이언트는 진행률·결과를 전혀 받지 못한다(HUD 게이지가 죽는다).
		if (!(*FoundAnchor)->GetIsReplicated())
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 목표 배치물이 복제되지 않아 클라이언트에 목표 진행이 전달되지 않습니다: 앵커=%s"),
				*EncounterId.ToString(),
				*GetNameSafe(*FoundAnchor));
		}

		ULastFPSTimedObjectiveComponent* Objective = Definition->CreateRuntimeObjective(**FoundAnchor);
		if (!Objective)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 런타임 목표 생성에 실패했습니다: 정의=%s"),
				*EncounterId.ToString(),
				*GetNameSafe(Definition));
			continue;
		}

		Objective->OnObjectiveResolved.AddUniqueDynamic(
			this,
			&ALastFPSRoomEncounterRuntime::HandleObjectiveResolved);
		Objectives.Add(Objective);
	}
}

bool ALastFPSRoomEncounterRuntime::AreAllObjectivesSucceeded() const
{
	// 목표가 없으면 참 — 섬멸형 인카운터의 기존 동작을 그대로 유지한다.
	for (const ULastFPSTimedObjectiveComponent* Objective : Objectives)
	{
		if (IsValid(Objective) && !Objective->IsSucceeded())
		{
			return false;
		}
	}
	return true;
}

void ALastFPSRoomEncounterRuntime::StartObjectives()
{
	for (ULastFPSTimedObjectiveComponent* Objective : Objectives)
	{
		if (IsValid(Objective))
		{
			Objective->StartObjective();
		}
	}
}

void ALastFPSRoomEncounterRuntime::StopObjectives()
{
	for (ULastFPSTimedObjectiveComponent* Objective : Objectives)
	{
		if (IsValid(Objective))
		{
			Objective->StopObjective();
		}
	}
}

void ALastFPSRoomEncounterRuntime::HandleObjectiveResolved(
	UActorComponent* /*Objective*/,
	const ELastFPSObjectiveResult ObjectiveResult)
{
	if (!HasAuthority() || bEncounterCleared)
	{
		return;
	}

	if (ObjectiveResult == ELastFPSObjectiveResult::Failed)
	{
		FailEncounter();
		return;
	}

	// 버티기 타이머가 적 전멸보다 먼저 끝날 수 있으므로 완료 조건을 다시 평가한다.
	EvaluateWaveCompletion();
}

void ALastFPSRoomEncounterRuntime::BeginEnemyDefinitionPreload()
{
	CancelEnemyDefinitionPreload();
	LoadedEnemyDefinitions.Reset();
	bEnemyDefinitionsReady = false;
	bEnemyDefinitionsFailed = false;

	TArray<FSoftObjectPath> DefinitionPaths;
	DefinitionPaths.Reserve(EnemyDefinitionAssets.Num());
	for (const TSoftObjectPtr<ULastFPSCharacterDefinition>& DefinitionAsset
		: EnemyDefinitionAssets)
	{
		const FSoftObjectPath DefinitionPath = DefinitionAsset.ToSoftObjectPath();
		if (DefinitionPath.IsValid())
		{
			DefinitionPaths.AddUnique(DefinitionPath);
		}
	}

	if (DefinitionPaths.IsEmpty())
	{
		bEnemyDefinitionsFailed = true;
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 비동기로 준비할 적 Character Definition 경로가 없습니다."),
			*EncounterId.ToString());
		return;
	}

	const bool bAllDefinitionsLoaded = Algo::AllOf(
		EnemyDefinitionAssets,
		[](const TSoftObjectPtr<ULastFPSCharacterDefinition>& DefinitionAsset)
		{
			return DefinitionAsset.Get() != nullptr;
		});
	if (bAllDefinitionsLoaded)
	{
		HandleEnemyDefinitionPreloadCompleted();
		return;
	}

	EnemyDefinitionLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		DefinitionPaths,
		FStreamableDelegate::CreateUObject(
			this,
			&ALastFPSRoomEncounterRuntime::HandleEnemyDefinitionPreloadCompleted),
		FStreamableManager::AsyncLoadHighPriority);
	if (!EnemyDefinitionLoadHandle.IsValid())
	{
		bEnemyDefinitionsFailed = true;
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 적 Character Definition 비동기 로드 요청을 시작하지 못했습니다."),
			*EncounterId.ToString());
	}
}

void ALastFPSRoomEncounterRuntime::HandleEnemyDefinitionPreloadCompleted()
{
	bEnemyDefinitionsReady = ResolveLoadedEnemyDefinitions();
	bEnemyDefinitionsFailed = !bEnemyDefinitionsReady;

	if (!bEnemyDefinitionsReady)
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 적 Character Definition 비동기 로드 결과가 유효하지 않습니다."),
			*EncounterId.ToString());
		if (HasAuthority() && bStartRequested)
		{
			bStartRequested = false;
			AbortEncounterOnConfigurationError(TEXT("적 Character Definition 비동기 로드 결과가 유효하지 않습니다."));
		}
		return;
	}

	UE_LOG(
		LogLastFPSRoomEncounter,
		Verbose,
		TEXT("[%s] 적 Character Definition %d개를 비동기로 준비했습니다."),
		*EncounterId.ToString(),
		LoadedEnemyDefinitions.Num());

	if (HasAuthority() && bStartRequested)
	{
		StartEncounter();
	}
}

void ALastFPSRoomEncounterRuntime::CancelEnemyDefinitionPreload()
{
	if (EnemyDefinitionLoadHandle.IsValid())
	{
		EnemyDefinitionLoadHandle->CancelHandle();
		EnemyDefinitionLoadHandle.Reset();
	}
}

bool ALastFPSRoomEncounterRuntime::ResolveLoadedEnemyDefinitions()
{
	LoadedEnemyDefinitions.Reset();
	for (const TSoftObjectPtr<ULastFPSCharacterDefinition>& DefinitionAsset
		: EnemyDefinitionAssets)
	{
		ULastFPSCharacterDefinition* Definition = DefinitionAsset.Get();
		if (!Definition || !Definition->PawnClass)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 적 Character Definition 또는 PawnClass가 유효하지 않습니다: %s"),
				*EncounterId.ToString(),
				*DefinitionAsset.ToString());
			return false;
		}

		LoadedEnemyDefinitions.AddUnique(Definition);
	}
	return !LoadedEnemyDefinitions.IsEmpty();
}

void ALastFPSRoomEncounterRuntime::OnRep_EnemyDefinitionAssets()
{
	if (!HasAuthority())
	{
		BeginEnemyDefinitionPreload();
	}
}

void ALastFPSRoomEncounterRuntime::ScheduleNextWave()
{
	if (!HasAuthority() || bEncounterCleared || bWaveSpawning)
	{
		return;
	}

	if (!Waves.IsValidIndex(CurrentWave))
	{
		CompleteEncounter();
		return;
	}

	const float Delay = FMath::Max(Waves[CurrentWave].DelayBeforeWave, 0.f);
	if (Delay > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(
			NextWaveTimerHandle,
			this,
			&ALastFPSRoomEncounterRuntime::SpawnNextWave,
			Delay,
			false);
		return;
	}

	SpawnNextWave();
}

void ALastFPSRoomEncounterRuntime::SpawnNextWave()
{
	if (!HasAuthority() || bEncounterCleared || !Waves.IsValidIndex(CurrentWave))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	ActiveWaveIndex = CurrentWave;
	++CurrentWave;
	NextSpawnUnitIndex = 0;
	SpawnedCountInCurrentUnit = 0;
	CurrentWaveSpawnPointIndex = 0;
	CurrentWaveSpawnedCount = 0;
	bWaveSpawning = true;
	PrepareSpawnPointOrder(Waves[ActiveWaveIndex]);
	ForceNetUpdate();

	SpawnNextQueuedEnemy();
}

void ALastFPSRoomEncounterRuntime::SpawnNextQueuedEnemy()
{
	if (!HasAuthority() || bEncounterCleared || !bWaveSpawning)
	{
		return;
	}

	if (!PendingEnemySpawns.IsEmpty()
		|| GetWorldTimerManager().IsTimerActive(SpawnPresentationDelayTimerHandle))
	{
		return;
	}

	const FLastFPSRoomEncounterWaveDefinition& Wave = Waves[ActiveWaveIndex];
	const int32 SpawnBatchSize = FMath::Max(Wave.SpawnBatchSize, 1);
	bool bQueuedSpawn = false;
	for (int32 BatchIndex = 0; BatchIndex < SpawnBatchSize; ++BatchIndex)
	{
		if (!QueueOneEnemySpawn())
		{
			break;
		}
		bQueuedSpawn = true;
	}

	if (!bQueuedSpawn)
	{
		FinishWaveSpawning();
		return;
	}

	if (SpawnDelayAfterVFX > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(
			SpawnPresentationDelayTimerHandle,
			this,
			&ALastFPSRoomEncounterRuntime::CommitPendingEnemySpawns,
			SpawnDelayAfterVFX,
			false);
		return;
	}

	CommitPendingEnemySpawns();
}

bool ALastFPSRoomEncounterRuntime::QueueOneEnemySpawn()
{
	if (!NormalizeSpawnCursor())
	{
		return false;
	}

	const FLastFPSRoomEncounterWaveDefinition& Wave = Waves[ActiveWaveIndex];
	const FLastFPSRoomEncounterUnitEntry& Unit = Wave.Units[NextSpawnUnitIndex];
	ULastFPSCharacterDefinition* Definition = Unit.EnemyDefinition.Get();
	if (!Definition)
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 지연 생성을 준비할 Character Definition을 찾지 못했습니다: Wave=%d, SpawnIndex=%d"),
			*EncounterId.ToString(),
			CurrentWave,
			CurrentWaveSpawnPointIndex);
		return false;
	}

	FLastFPSPendingRoomEnemySpawn& PendingSpawn = PendingEnemySpawns.AddDefaulted_GetRef();
	PendingSpawn.Definition = Definition;
	PendingSpawn.SpawnIndex = CurrentWaveSpawnPointIndex;
	PendingSpawn.SpawnTransform = ResolveSpawnTransform(CurrentWaveSpawnPointIndex);
	SpawnPresentationComponent->PlaySpawnVFX(PendingSpawn.SpawnTransform);

	++SpawnedCountInCurrentUnit;
	++CurrentWaveSpawnPointIndex;
	return true;
}

void ALastFPSRoomEncounterRuntime::CommitPendingEnemySpawns()
{
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	if (!HasAuthority() || bEncounterCleared || !bWaveSpawning)
	{
		PendingEnemySpawns.Reset();
		return;
	}

	const int32 SpawnCountThisFrame = FMath::Min(
		PendingEnemySpawns.Num(),
		MaxSpawnedActorsPerFrame);
	for (int32 SpawnIndexInFrame = 0;
		SpawnIndexInFrame < SpawnCountThisFrame;
		++SpawnIndexInFrame)
	{
		const FLastFPSPendingRoomEnemySpawn PendingSpawn = PendingEnemySpawns[0];
		PendingEnemySpawns.RemoveAt(0, 1, EAllowShrinking::No);
		ULastFPSCharacterDefinition* Definition = PendingSpawn.Definition.Get();
		if (!Definition)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 지연 생성 대기 중 Character Definition이 유효하지 않게 되었습니다: SpawnIndex=%d"),
				*EncounterId.ToString(),
				PendingSpawn.SpawnIndex);
			continue;
		}

		if (ALastFPSCharacterBase* SpawnedEnemy = SpawnEnemy(
			*Definition,
			PendingSpawn.SpawnTransform,
			PendingSpawn.SpawnIndex))
		{
			AliveEnemies.Add(SpawnedEnemy);
			++CurrentWaveSpawnedCount;
		}
	}

	if (!PendingEnemySpawns.IsEmpty())
	{
		SpawnPresentationDelayTimerHandle = GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ALastFPSRoomEncounterRuntime::CommitPendingEnemySpawns);
		return;
	}

	if (!NormalizeSpawnCursor())
	{
		FinishWaveSpawning();
		return;
	}

	ScheduleNextSpawnBatch();
}

void ALastFPSRoomEncounterRuntime::ScheduleNextSpawnBatch()
{
	if (!Waves.IsValidIndex(ActiveWaveIndex))
	{
		FinishWaveSpawning();
		return;
	}

	const float SpawnInterval = FMath::Max(Waves[ActiveWaveIndex].SpawnInterval, 0.f);
	if (SpawnInterval <= KINDA_SMALL_NUMBER)
	{
		SequentialSpawnTimerHandle = GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ALastFPSRoomEncounterRuntime::SpawnNextQueuedEnemy);
		return;
	}

	GetWorldTimerManager().SetTimer(
		SequentialSpawnTimerHandle,
		this,
		&ALastFPSRoomEncounterRuntime::SpawnNextQueuedEnemy,
		SpawnInterval,
		false);
}

bool ALastFPSRoomEncounterRuntime::NormalizeSpawnCursor()
{
	if (!Waves.IsValidIndex(ActiveWaveIndex))
	{
		return false;
	}

	const TArray<FLastFPSRoomEncounterUnitEntry>& Units = Waves[ActiveWaveIndex].Units;
	while (Units.IsValidIndex(NextSpawnUnitIndex)
		&& SpawnedCountInCurrentUnit >= Units[NextSpawnUnitIndex].Count)
	{
		++NextSpawnUnitIndex;
		SpawnedCountInCurrentUnit = 0;
	}

	return Units.IsValidIndex(NextSpawnUnitIndex);
}

void ALastFPSRoomEncounterRuntime::PrepareSpawnPointOrder(
	const FLastFPSRoomEncounterWaveDefinition& Wave)
{
	ActiveSpawnPointOrder.SetNumUninitialized(SpawnPoints.Num());
	for (int32 PointIndex = 0; PointIndex < ActiveSpawnPointOrder.Num(); ++PointIndex)
	{
		ActiveSpawnPointOrder[PointIndex] = PointIndex;
	}

	if (!Wave.bShuffleSpawnPoints)
	{
		return;
	}

	for (int32 PointIndex = ActiveSpawnPointOrder.Num() - 1; PointIndex > 0; --PointIndex)
	{
		const int32 SwapIndex = FMath::RandRange(0, PointIndex);
		ActiveSpawnPointOrder.Swap(PointIndex, SwapIndex);
	}
}

void ALastFPSRoomEncounterRuntime::FinishWaveSpawning()
{
	if (!bWaveSpawning)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	bWaveSpawning = false;
	ActiveWaveIndex = INDEX_NONE;
	ActiveSpawnPointOrder.Reset();
	PendingEnemySpawns.Reset();

	UE_LOG(
		LogLastFPSRoomEncounter,
		Log,
		TEXT("[%s] 웨이브 %d/%d: 적 %d마리를 생성했습니다."),
		*EncounterId.ToString(),
		CurrentWave,
		Waves.Num(),
		CurrentWaveSpawnedCount);

	if (CurrentWaveSpawnedCount == 0)
	{
		AbortEncounterOnConfigurationError(TEXT("웨이브에서 적을 한 마리도 생성하지 못했습니다."));
		return;
	}

	EvaluateWaveCompletion();
}

ALastFPSCharacterBase* ALastFPSRoomEncounterRuntime::SpawnEnemy(
	ULastFPSCharacterDefinition& Definition,
	const FTransform& SpawnTransform,
	const int32 SpawnIndex)
{
	UWorld* World = GetWorld();
	if (!Definition.PawnClass || !World)
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 적 생성 준비가 유효하지 않습니다: Definition=%s, World=%s"),
			*EncounterId.ToString(),
			*GetNameSafe(&Definition),
			*GetNameSafe(World));
		return nullptr;
	}

	APawn* SpawnedPawn = nullptr;
	bool bAcquiredFromPool = false;
	if (ULastFPSActorPoolSubsystem* Pool =
		World->GetSubsystem<ULastFPSActorPoolSubsystem>())
	{
		SpawnedPawn = Cast<APawn>(Pool->AcquireActorByClass(
			Definition.PawnClass,
			SpawnTransform,
			this,
			nullptr));
		bAcquiredFromPool = SpawnedPawn != nullptr;
	}
	if (!SpawnedPawn)
	{
		SpawnedPawn = World->SpawnActorDeferred<APawn>(
			Definition.PawnClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	}

	ALastFPSCharacterBase* SpawnedEnemy = Cast<ALastFPSCharacterBase>(SpawnedPawn);
	if (!SpawnedPawn || !SpawnedEnemy)
	{
		if (SpawnedPawn)
		{
			if (ULastFPSActorPoolSubsystem* Pool =
				World->GetSubsystem<ULastFPSActorPoolSubsystem>();
				!Pool || !Pool->ReleaseActor(SpawnedPawn))
			{
				SpawnedPawn->Destroy();
			}
		}
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 웨이브 %d의 적 생성에 실패했습니다: SpawnIndex=%d, Definition=%s"),
			*EncounterId.ToString(),
			CurrentWave,
			SpawnIndex,
			*GetNameSafe(&Definition));
		return nullptr;
	}

	if (bAcquiredFromPool)
	{
		SpawnedEnemy->ResetForPoolReuse(&Definition);

		// 일반 Spawn의 AdjustIfPossibleButAlwaysSpawn은 컴포넌트 초기화 뒤 Capsule 겹침을 보정한다.
		// 풀 Actor는 이미 BeginPlay를 마쳤으므로 상태 복구로 충돌을 켠 다음 같은 보정을 명시적으로 수행한다.
		FVector AdjustedLocation = SpawnTransform.GetLocation();
		const FRotator AdjustedRotation = SpawnTransform.Rotator();
		if (World->FindTeleportSpot(
			SpawnedPawn,
			AdjustedLocation,
			AdjustedRotation))
		{
			SpawnedPawn->SetActorLocationAndRotation(
				AdjustedLocation,
				AdjustedRotation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		SpawnedEnemy->SetCharacterDefinitionForSpawn(&Definition);
		SpawnedPawn->FinishSpawning(SpawnTransform);
	}
	SpawnedPawn->SpawnDefaultController();
	SpawnedEnemy->OnDeath.AddUObject(
		this,
		&ALastFPSRoomEncounterRuntime::HandleEnemyDeath);
	SpawnedEnemy->OnDestroyed.AddUniqueDynamic(
		this,
		&ALastFPSRoomEncounterRuntime::HandleEnemyDestroyed);
	return SpawnedEnemy;
}

void ALastFPSRoomEncounterRuntime::HandleEnemyDeath(ALastFPSCharacterBase* DeadCharacter)
{
	if (!HasAuthority() || !IsValid(DeadCharacter))
	{
		return;
	}

	RemoveTrackedEnemy(*DeadCharacter);
	EvaluateWaveCompletion();
}

void ALastFPSRoomEncounterRuntime::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	ALastFPSCharacterBase* DestroyedEnemy = Cast<ALastFPSCharacterBase>(DestroyedActor);
	if (!DestroyedEnemy
		|| !AliveEnemies.Contains(TWeakObjectPtr<ALastFPSCharacterBase>(DestroyedEnemy)))
	{
		return;
	}

	RemoveTrackedEnemy(*DestroyedEnemy);
	EvaluateWaveCompletion();
}

void ALastFPSRoomEncounterRuntime::RemoveTrackedEnemy(
	ALastFPSCharacterBase& Enemy,
	const bool bCountAsDefeated)
{
	Enemy.OnDeath.RemoveAll(this);
	Enemy.OnDestroyed.RemoveDynamic(
		this,
		&ALastFPSRoomEncounterRuntime::HandleEnemyDestroyed);
	const int32 RemovedCount = AliveEnemies.Remove(TWeakObjectPtr<ALastFPSCharacterBase>(&Enemy));
	if (RemovedCount > 0 && bCountAsDefeated)
	{
		DefeatedEnemyCount = FMath::Min(DefeatedEnemyCount + 1, TotalEnemyCount);
		BroadcastEncounterProgress();
		ForceNetUpdate();
	}
}

void ALastFPSRoomEncounterRuntime::BroadcastEncounterProgress() const
{
	if (const UWorld* World = GetWorld())
	{
		if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
			World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
		{
			EncounterSubsystem->NotifyEncounterProgress(
				EncounterId,
				DefeatedEnemyCount,
				TotalEnemyCount);
		}
	}
}

void ALastFPSRoomEncounterRuntime::OnRep_EncounterProgress()
{
	BroadcastEncounterProgress();
}

void ALastFPSRoomEncounterRuntime::EvaluateWaveCompletion()
{
	// 목표가 먼저 성공하면 남은 적을 정리하고 즉시 끝낸다 — 배리어가 열린 뒤에도
	// 적이 따라 나오는 상황을 막는다.
	if (!bEncounterCleared && !Objectives.IsEmpty() && AreAllObjectivesSucceeded())
	{
		ClearAliveEnemies();
		CompleteEncounter();
		return;
	}

	if (bWaveSpawning || !AliveEnemies.IsEmpty() || bEncounterCleared)
	{
		return;
	}

	if (CurrentWave < Waves.Num())
	{
		ScheduleNextWave();
		return;
	}

	// 웨이브를 다 소진했는데 목표가 남았다면 처음부터 다시 몰아친다.
	if (!AreAllObjectivesSucceeded())
	{
		CurrentWave = 0;
		++WaveLoopCount;
		// 분모도 한 바퀴분 늘려야 처치 수 기반 진행 표시가 100%에 고정되지 않는다.
		TotalEnemyCount += BaseTotalEnemyCount;
		BroadcastEncounterProgress();
		ForceNetUpdate();
		UE_LOG(
			LogLastFPSRoomEncounter,
			Verbose,
			TEXT("[%s] 목표가 남아 웨이브를 순환합니다 (%d회차)."),
			*EncounterId.ToString(),
			WaveLoopCount);
		ScheduleNextWave();
		return;
	}

	CompleteEncounter();
}

#if !UE_BUILD_SHIPPING
bool ALastFPSRoomEncounterRuntime::DebugForceCompleteEncounter()
{
	// 미진입 구역을 완료하면 순차 퀘스트의 도달 목표보다 클리어 이벤트가 먼저 소모될 수 있다.
	if (!HasAuthority() || !bInitialized || !bEncounterStarted)
	{
		return false;
	}

	if (bEncounterCleared)
	{
		return true;
	}

	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	PendingEnemySpawns.Reset();
	bWaveSpawning = false;

	// 파괴 콜백이 실제 처치로 중복 집계되지 않도록 추적을 먼저 해제한다.
	for (const TWeakObjectPtr<ALastFPSCharacterBase>& EnemyPtr : AliveEnemies)
	{
		if (ALastFPSCharacterBase* Enemy = EnemyPtr.Get())
		{
			Enemy->OnDeath.RemoveAll(this);
			Enemy->OnDestroyed.RemoveDynamic(
				this,
				&ALastFPSRoomEncounterRuntime::HandleEnemyDestroyed);
			ULastFPSActorPoolSubsystem* Pool = GetWorld()
				? GetWorld()->GetSubsystem<ULastFPSActorPoolSubsystem>()
				: nullptr;
			if (!Pool || !Pool->ReleaseActor(Enemy))
			{
				Enemy->Destroy();
			}
		}
	}
	AliveEnemies.Reset();

	if (IsValid(TriggerVolume))
	{
		TriggerVolume->SetActorEnableCollision(false);
	}

	bEncounterStarted = true;
	CurrentWave = Waves.Num();
	DefeatedEnemyCount = TotalEnemyCount;
	BroadcastEncounterProgress();

	// 목표를 남겨 두면 방은 열리는데 퀘스트는 미해결로 남아 건너뛰기가 반쪽이 된다.
	for (ULastFPSTimedObjectiveComponent* Objective : Objectives)
	{
		if (IsValid(Objective))
		{
			Objective->DebugForceSucceed();
		}
	}

	CompleteEncounter();
	return bEncounterCleared;
}
#endif

void ALastFPSRoomEncounterRuntime::CompleteEncounter()
{
	if (!HasAuthority() || bEncounterCleared)
	{
		return;
	}

	bEncounterCleared = true;
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	PendingEnemySpawns.Reset();
	bWaveSpawning = false;
	StopObjectives();

	// 목표 성공으로 조기 종료한 경우 남은 적이 처치로 집계되지 않으므로 진행 표시가
	// 100%에 못 미친 채 끝난다. 클리어는 클리어이므로 표시를 맞춰 준다(디버그 경로와 동일).
	DefeatedEnemyCount = TotalEnemyCount;
	BroadcastEncounterProgress();

	SetBarrierActive(false);
	ForceNetUpdate();
	UE_LOG(
		LogLastFPSRoomEncounter,
		Log,
		TEXT("[%s] 인카운터를 완료했습니다 (순환 %d회)."),
		*EncounterId.ToString(),
		WaveLoopCount);

	if (const UWorld* World = GetWorld())
	{
		if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem = World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
		{
			EncounterSubsystem->NotifyEncounterCleared(EncounterId);
		}
	}
}

void ALastFPSRoomEncounterRuntime::FailEncounter()
{
	if (!HasAuthority() || bEncounterCleared)
	{
		return;
	}

	// 실패도 전투 종료다 — 클리어 플래그를 세워 이후 웨이브·재평가를 모두 막는다.
	bEncounterCleared = true;
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	PendingEnemySpawns.Reset();
	bWaveSpawning = false;
	StopObjectives();
	ClearAliveEnemies();
	SetBarrierActive(false);
	ForceNetUpdate();

	UE_LOG(
		LogLastFPSRoomEncounter,
		Log,
		TEXT("[%s] 목표 실패로 인카운터를 종료합니다."),
		*EncounterId.ToString());

	// 실패 이후 처리(미션 실패 연출·귀환)는 맵마다 다른 규칙이라 GameMode 가 소유한다.
	if (const UWorld* World = GetWorld())
	{
		if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem = World->GetSubsystem<ULastFPSRoomEncounterSubsystem>())
		{
			EncounterSubsystem->NotifyEncounterFailed(EncounterId);
		}
	}
}

void ALastFPSRoomEncounterRuntime::ClearAliveEnemies()
{
	if (!HasAuthority())
	{
		return;
	}

	ULastFPSActorPoolSubsystem* Pool = GetWorld()
		? GetWorld()->GetSubsystem<ULastFPSActorPoolSubsystem>()
		: nullptr;

	// 순회 중 RemoveTrackedEnemy 가 집합을 수정하므로 스냅샷을 뜬다.
	TArray<TWeakObjectPtr<ALastFPSCharacterBase>> Snapshot = AliveEnemies.Array();
	for (const TWeakObjectPtr<ALastFPSCharacterBase>& EnemyPtr : Snapshot)
	{
		ALastFPSCharacterBase* Enemy = EnemyPtr.Get();
		if (!IsValid(Enemy))
		{
			continue;
		}

		// 강제 정리는 처치가 아니다 — 집계에 넣으면 퀘스트 진행이 부풀어 오른다.
		RemoveTrackedEnemy(*Enemy, /*bCountAsDefeated=*/false);

		// 스폰 경로와 동일하게 풀에 먼저 반납한다. 풀 소속이 아니면 파괴한다.
		if (!Pool || !Pool->ReleaseActor(Enemy))
		{
			Enemy->Destroy();
		}
	}
	AliveEnemies.Reset();
}

void ALastFPSRoomEncounterRuntime::AbortEncounterOnConfigurationError(const TCHAR* Reason)
{
	UE_LOG(
		LogLastFPSRoomEncounter,
		Error,
		TEXT("[%s] 전투 진행 실패로 출구를 개방합니다. 원인: %s"),
		*EncounterId.ToString(),
		Reason);
	bEncounterCleared = true;
	GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SequentialSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnPresentationDelayTimerHandle);
	PendingEnemySpawns.Reset();
	bWaveSpawning = false;
	SetBarrierActive(false);
	ForceNetUpdate();
}

void ALastFPSRoomEncounterRuntime::SetBarrierActive(const bool bNewActive)
{
	bBarrierActive = bNewActive;
	ApplyBarrierState();
	ForceNetUpdate();
}

void ALastFPSRoomEncounterRuntime::ConfigureBarrierPresentation()
{
	// 전용 서버는 시각 에셋을 로드하거나 짧은 소거 Tick을 실행할 필요가 없다.
	ResetBarrierPresentations();
	CancelBarrierPresentationLoad();
	if (GetNetMode() == NM_DedicatedServer
		|| BarrierVolumes.IsEmpty()
		|| BarrierPresentationSettings.Mode == ELastFPSRoomBarrierPresentationMode::None)
	{
		return;
	}

	LoadedBarrierMesh = BarrierPresentationSettings.Mesh.Get();
	LoadedBarrierMaterial = BarrierPresentationSettings.Material.Get();
	if (!LoadedBarrierMesh || !LoadedBarrierMaterial)
	{
		BeginBarrierPresentationLoad();
		return;
	}

	for (ATriggerBox* BarrierVolume : BarrierVolumes)
	{
		if (!IsValid(BarrierVolume))
		{
			continue;
		}

		const FName ComponentName = MakeUniqueObjectName(
			this,
			ULastFPSRoomBarrierPresentationComponent::StaticClass(),
			TEXT("BarrierPresentationComponent"));
		ULastFPSRoomBarrierPresentationComponent* PresentationComponent =
			NewObject<ULastFPSRoomBarrierPresentationComponent>(this, ComponentName);
		if (!PresentationComponent)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Error,
				TEXT("[%s] 배리어 '%s'의 시각 컴포넌트를 생성하지 못했습니다."),
				*EncounterId.ToString(),
				*BarrierVolume->GetPathName());
			continue;
		}

		AddInstanceComponent(PresentationComponent);
		PresentationComponent->SetupAttachment(SceneRoot);
		PresentationComponent->RegisterComponent();
		PresentationComponent->Configure(
			*BarrierVolume,
			BarrierPresentationSettings,
			*LoadedBarrierMesh,
			*LoadedBarrierMaterial);
		BarrierPresentationComponents.Add(PresentationComponent);
	}
}

void ALastFPSRoomEncounterRuntime::BeginBarrierPresentationLoad()
{
	if (BarrierPresentationSettings.Mode
		!= ELastFPSRoomBarrierPresentationMode::Mesh)
	{
		return;
	}

	TArray<FSoftObjectPath> PresentationPaths;
	const FSoftObjectPath MeshPath =
		BarrierPresentationSettings.Mesh.ToSoftObjectPath();
	const FSoftObjectPath MaterialPath =
		BarrierPresentationSettings.Material.ToSoftObjectPath();
	if (MeshPath.IsValid())
	{
		PresentationPaths.Add(MeshPath);
	}
	if (MaterialPath.IsValid())
	{
		PresentationPaths.AddUnique(MaterialPath);
	}

	if (PresentationPaths.Num() != 2)
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 배리어 Mesh 또는 Material 경로가 유효하지 않습니다. Mesh=%s, Material=%s"),
			*EncounterId.ToString(),
			*MeshPath.ToString(),
			*MaterialPath.ToString());
		return;
	}

	BarrierPresentationLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PresentationPaths,
		FStreamableDelegate::CreateUObject(
			this,
			&ALastFPSRoomEncounterRuntime::HandleBarrierPresentationAssetsLoaded),
		FStreamableManager::AsyncLoadHighPriority);
	if (!BarrierPresentationLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 배리어 시각 에셋의 비동기 로드를 시작하지 못했습니다."),
			*EncounterId.ToString());
	}
}

void ALastFPSRoomEncounterRuntime::HandleBarrierPresentationAssetsLoaded()
{
	LoadedBarrierMesh = BarrierPresentationSettings.Mesh.Get();
	LoadedBarrierMaterial = BarrierPresentationSettings.Material.Get();
	BarrierPresentationLoadHandle.Reset();
	if (!LoadedBarrierMesh || !LoadedBarrierMaterial)
	{
		UE_LOG(
			LogLastFPSRoomEncounter,
			Error,
			TEXT("[%s] 배리어 시각 에셋의 비동기 로드 결과가 유효하지 않습니다."),
			*EncounterId.ToString());
		return;
	}

	ConfigureBarrierPresentation();
	ApplyBarrierState();
}

void ALastFPSRoomEncounterRuntime::CancelBarrierPresentationLoad()
{
	if (BarrierPresentationLoadHandle.IsValid())
	{
		BarrierPresentationLoadHandle->CancelHandle();
		BarrierPresentationLoadHandle.Reset();
	}
}

void ALastFPSRoomEncounterRuntime::ResetBarrierPresentations()
{
	for (ULastFPSRoomBarrierPresentationComponent* PresentationComponent
		: BarrierPresentationComponents)
	{
		if (IsValid(PresentationComponent))
		{
			PresentationComponent->DestroyComponent();
		}
	}
	BarrierPresentationComponents.Reset();
}

void ALastFPSRoomEncounterRuntime::ApplyBarrierState()
{
	for (ATriggerBox* BarrierVolume : BarrierVolumes)
	{
		if (!IsValid(BarrierVolume))
		{
			continue;
		}

		UBoxComponent* BarrierBox = BarrierVolume->FindComponentByClass<UBoxComponent>();
		if (!BarrierBox)
		{
			continue;
		}

		BarrierBox->SetGenerateOverlapEvents(false);
		BarrierBox->SetCollisionResponseToAllChannels(ECR_Block);
		BarrierBox->SetCollisionEnabled(
			bBarrierActive
				? ECollisionEnabled::QueryAndPhysics
				: ECollisionEnabled::NoCollision);
	}

	for (ULastFPSRoomBarrierPresentationComponent* PresentationComponent
		: BarrierPresentationComponents)
	{
		if (IsValid(PresentationComponent))
		{
			PresentationComponent->SetBarrierActive(bBarrierActive);
		}
	}
}

void ALastFPSRoomEncounterRuntime::OnRep_BarrierState()
{
	ApplyBarrierState();
}

void ALastFPSRoomEncounterRuntime::OnRep_BarrierVolumes()
{
	ConfigureBarrierPresentation();
	ApplyBarrierState();
}

void ALastFPSRoomEncounterRuntime::OnRep_BarrierPresentationSettings()
{
	ConfigureBarrierPresentation();
	ApplyBarrierState();
}

FTransform ALastFPSRoomEncounterRuntime::ResolveSpawnTransform(const int32 SpawnIndex)
{
	const int32 PointCount = SpawnPoints.Num();
	if (PointCount <= 0)
	{
		return FTransform::Identity;
	}

	const int32 PointOrderIndex = SpawnIndex % PointCount;
	const int32 PointIndex = ActiveSpawnPointOrder.IsValidIndex(PointOrderIndex)
		? ActiveSpawnPointOrder[PointOrderIndex]
		: PointOrderIndex;
	const int32 ReuseIndex = SpawnIndex / PointCount;
	FTransform SpawnTransform = SpawnPoints[PointIndex]->GetActorTransform();
	if (SpawnPointRandomRadius > KINDA_SMALL_NUMBER)
	{
		if (UNavigationSystemV1* NavigationSystem =
			FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			const FVector SpawnPointLocation = SpawnTransform.GetLocation();
			FNavLocation RandomNavLocation;
			if (NavigationSystem->GetRandomReachablePointInRadius(
				SpawnPointLocation,
				SpawnPointRandomRadius,
				RandomNavLocation))
			{
				FVector RandomSpawnLocation = RandomNavLocation.Location;
				FNavLocation ProjectedSpawnPoint;
				if (NavigationSystem->ProjectPointToNavigation(
					SpawnPointLocation,
					ProjectedSpawnPoint))
				{
					RandomSpawnLocation.Z += SpawnPointLocation.Z - ProjectedSpawnPoint.Location.Z;
				}
				else
				{
					RandomSpawnLocation.Z = SpawnPointLocation.Z;
				}

				SpawnTransform.SetLocation(RandomSpawnLocation);
				return SpawnTransform;
			}
		}

		if (!bLoggedSpawnProjectionFailure)
		{
			UE_LOG(
				LogLastFPSRoomEncounter,
				Warning,
				TEXT("[%s] Spawn Point 주변의 도달 가능한 NavMesh 위치를 찾지 못해 원래 위치를 사용합니다."),
				*EncounterId.ToString());
			bLoggedSpawnProjectionFailure = true;
		}
	}

	if (ReuseIndex <= 0 || ReusedSpawnPointSpacing <= 0.f)
	{
		return SpawnTransform;
	}

	constexpr float GoldenAngleDegrees = 137.507764f;
	const float PointAngleDegrees =
		360.f * static_cast<float>(PointOrderIndex) / static_cast<float>(PointCount);
	const float AngleRadians = FMath::DegreesToRadians(
		PointAngleDegrees + GoldenAngleDegrees * static_cast<float>(ReuseIndex));
	const float OffsetRadius =
		ReusedSpawnPointSpacing * FMath::Sqrt(static_cast<float>(ReuseIndex));
	const FVector LocalOffset(
		FMath::Cos(AngleRadians) * OffsetRadius,
		FMath::Sin(AngleRadians) * OffsetRadius,
		0.f);
	SpawnTransform.AddToTranslation(SpawnTransform.GetRotation().RotateVector(LocalOffset));
	return SpawnTransform;
}
