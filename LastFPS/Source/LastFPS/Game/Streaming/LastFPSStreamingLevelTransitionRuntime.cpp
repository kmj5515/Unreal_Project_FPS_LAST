#include "Game/Streaming/LastFPSStreamingLevelTransitionRuntime.h"

#include "AbilitySystemComponent.h"
#include "Character/LastFPSCharacterBase.h"
#include "Character/LastFPSEnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Data/Definitions/LastFPSEnemyDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/PlayerStartPIE.h"
#include "Engine/StreamableManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "Net/UnrealNetwork.h"
#include "Encounter/LastFPSRoomEncounterSubsystem.h"
#include "Game/Streaming/LastFPSArrivalContainmentPresentationComponent.h"
#include "Materials/MaterialInterface.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "Quest/LastFPSQuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStreamingTransition, Log, All);

ALastFPSStreamingLevelTransitionRuntime::
	ALastFPSStreamingLevelTransitionRuntime()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	EncounterObjectiveMarker =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("EncounterObjectiveMarker"));
	SetRootComponent(EncounterObjectiveMarker);

	ArrivalContainmentPresentation =
		CreateDefaultSubobject<
			ULastFPSArrivalContainmentPresentationComponent>(
				TEXT("ArrivalContainmentPresentation"));
	ArrivalContainmentPresentation->SetupAttachment(
		EncounterObjectiveMarker);
}

void ALastFPSStreamingLevelTransitionRuntime::ConfigureRoute(
	const FLastFPSStreamingLevelTransitionRoute& InRoute,
	ATriggerBox& InTriggerVolume)
{
	check(HasAuthority());
	Route = InRoute;
	TriggerVolume = &InTriggerVolume;
}

void ALastFPSStreamingLevelTransitionRuntime::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALastFPSStreamingLevelTransitionRuntime, Route);
	DOREPLIFETIME(
		ALastFPSStreamingLevelTransitionRuntime,
		bTransitionRequested);
}

void ALastFPSStreamingLevelTransitionRuntime::BeginPlay()
{
	Super::BeginPlay();

	BeginDestinationPreload();
	BeginArrivalContainmentPreload();
	BeginDelayedEnemyContentPreload();

	if (HasAuthority() && IsValid(TriggerVolume))
	{
		TriggerVolume->OnActorBeginOverlap.AddUniqueDynamic(
			this,
			&ThisClass::HandleTriggerOverlap);
	}
}

void ALastFPSStreamingLevelTransitionRuntime::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(TriggerVolume))
	{
		TriggerVolume->OnActorBeginOverlap.RemoveDynamic(
			this,
			&ThisClass::HandleTriggerOverlap);
	}

	if (IsValid(DestinationStreamingLevel))
	{
		DestinationStreamingLevel->OnLevelLoaded.RemoveDynamic(
			this,
			&ThisClass::HandleDestinationLevelLoaded);
		DestinationStreamingLevel->OnLevelShown.RemoveDynamic(
			this,
			&ThisClass::HandleDestinationLevelShown);
	}

	GetWorldTimerManager().ClearTimer(DelayedEnemySpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ArrivalHoldTimerHandle);
	GetWorldTimerManager().ClearTimer(ArrivalContainmentTimerHandle);
	ReleaseArrivalHold();
	HideArrivalContainment();
	UnregisterEncounterObjectiveMarker();
	if (ALastFPSEnemyCharacter* SpawnedEnemy =
		SpawnedDelayedEnemy.Get())
	{
		SpawnedEnemy->OnDeath.RemoveAll(this);
	}
	CancelDelayedEnemyContentPreload();
	CancelArrivalContainmentPreload();
	LoadedDelayedEnemyDefinition = nullptr;
	LoadedArrivalContainmentMesh = nullptr;
	LoadedArrivalContainmentMaterial = nullptr;
	PendingPawn.Reset();
	SpawnedDelayedEnemy.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALastFPSStreamingLevelTransitionRuntime::HandleTriggerOverlap(
	AActor* /*OverlappedActor*/,
	AActor* OtherActor)
{
	if (!HasAuthority() || bTransitionRequested)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (!IsValid(DestinationStreamingLevel))
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 목적지 스트리밍 레벨이 없어 플레이어를 이동할 수 없습니다: Pawn=%s"),
			*Route.RouteId.ToString(),
			*GetNameSafe(Pawn));
		return;
	}

	PendingPawn = Pawn;
	bTransitionRequested = true;
	if (IsValid(TriggerVolume))
	{
		TriggerVolume->SetActorEnableCollision(false);
	}

	RequestDestinationVisibility();
	ForceNetUpdate();
}

void ALastFPSStreamingLevelTransitionRuntime::
	HandleDestinationLevelLoaded()
{
	if (bDestinationLoaded)
	{
		return;
	}

	bDestinationLoaded = true;
	UE_LOG(
		LogLastFPSStreamingTransition,
		Log,
		TEXT("[%s] 목적지 레벨을 숨김 상태로 준비했습니다: %s"),
		*Route.RouteId.ToString(),
		*Route.DestinationLevel.ToString());

	if (bTransitionRequested)
	{
		RequestDestinationVisibility();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::HandleDestinationLevelShown()
{
	UpdateEncounterObjectiveMarker();

	if (HasAuthority())
	{
		CompletePendingPawnTransition();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::OnRep_Route()
{
	BeginDestinationPreload();
	BeginArrivalContainmentPreload();
	BeginDelayedEnemyContentPreload();
}

void ALastFPSStreamingLevelTransitionRuntime::
	OnRep_TransitionRequested()
{
	if (bTransitionRequested)
	{
		RequestDestinationVisibility();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	Multicast_NotifyTransitionArrived_Implementation(
		const FVector_NetQuantize ArrivalLocation,
		const FRotator ArrivalRotation)
{
	if (Route.ArrivalLocationTag.IsValid())
	{
		const UWorld* World = GetWorld();
		UGameInstance* GameInstance = World
			? World->GetGameInstance()
			: nullptr;
		if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance
			? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>()
			: nullptr)
		{
			// 스트리밍 전환이 같은 TriggerBox의 오버랩을 먼저 끊어도 도착 목표를 확정한다.
			QuestSubsystem->NotifyLocationTriggerChanged(
				Route.ArrivalLocationTag,
				true);
			QuestSubsystem->NotifyLocationTriggerChanged(
				Route.ArrivalLocationTag,
				false);
		}
	}

	if (Route.ArrivalContainment.bEnabled
		&& ArrivalContainmentPresentation)
	{
		ArrivalContainmentPresentation->ShowAt(
			FTransform(
				ArrivalRotation,
				ArrivalLocation));
		GetWorldTimerManager().ClearTimer(
			ArrivalContainmentTimerHandle);
		GetWorldTimerManager().SetTimer(
			ArrivalContainmentTimerHandle,
			this,
			&ThisClass::HideArrivalContainment,
			FMath::Max(Route.ArrivalHoldDuration, 0.f),
			false);
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	Multicast_NotifyDelayedEnemyEncounterProgress_Implementation(
		const int32 DefeatedEnemyCount,
		const int32 TotalEnemyCount,
		const bool bEncounterCleared)
{
	if (Route.DelayedEnemyEncounterId.IsNone())
	{
		return;
	}

	if (ULastFPSRoomEncounterSubsystem* EncounterSubsystem =
		GetWorld()
			? GetWorld()->GetSubsystem<ULastFPSRoomEncounterSubsystem>()
			: nullptr)
	{
		EncounterSubsystem->NotifyEncounterProgress(
			Route.DelayedEnemyEncounterId,
			DefeatedEnemyCount,
			TotalEnemyCount);
		if (bEncounterCleared)
		{
			EncounterSubsystem->NotifyEncounterCleared(
				Route.DelayedEnemyEncounterId);
		}
	}
}

void ALastFPSStreamingLevelTransitionRuntime::BeginDestinationPreload()
{
	if (IsValid(DestinationStreamingLevel) || Route.DestinationLevel.IsNull())
	{
		return;
	}

	FString FailureReason;
	if (!Route.IsValid(FailureReason))
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("스트리밍 전환 경로가 유효하지 않습니다: Route=%s, 원인=%s"),
			*Route.RouteId.ToString(),
			*FailureReason);
		return;
	}

	bool bLoadRequestAccepted = false;
	DestinationStreamingLevel =
		ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
			this,
			Route.DestinationLevel,
			Route.DestinationLevelTransform,
			bLoadRequestAccepted,
			MakeStreamingInstanceName());
	if (!bLoadRequestAccepted || !IsValid(DestinationStreamingLevel))
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 목적지 레벨의 비동기 스트리밍 요청에 실패했습니다: %s"),
			*Route.RouteId.ToString(),
			*Route.DestinationLevel.ToString());
		DestinationStreamingLevel = nullptr;
		return;
	}

	DestinationStreamingLevel->OnLevelLoaded.AddUniqueDynamic(
		this,
		&ThisClass::HandleDestinationLevelLoaded);
	DestinationStreamingLevel->OnLevelShown.AddUniqueDynamic(
		this,
		&ThisClass::HandleDestinationLevelShown);
	DestinationStreamingLevel->SetShouldBeLoaded(true);
	DestinationStreamingLevel->SetShouldBeVisible(bTransitionRequested);

	if (DestinationStreamingLevel->IsLevelLoaded())
	{
		HandleDestinationLevelLoaded();
	}
	if (bTransitionRequested
		&& DestinationStreamingLevel->IsLevelVisible())
	{
		HandleDestinationLevelShown();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	BeginArrivalContainmentPreload()
{
	CancelArrivalContainmentPreload();
	LoadedArrivalContainmentMesh = nullptr;
	LoadedArrivalContainmentMaterial = nullptr;

	if (!Route.ArrivalContainment.bEnabled)
	{
		HideArrivalContainment();
		return;
	}

	LoadedArrivalContainmentMesh =
		Route.ArrivalContainment.Mesh.Get();
	LoadedArrivalContainmentMaterial =
		Route.ArrivalContainment.Material.Get();
	if (LoadedArrivalContainmentMesh
		&& LoadedArrivalContainmentMaterial)
	{
		HandleArrivalContainmentLoaded();
		return;
	}

	TArray<FSoftObjectPath> RequiredPaths;
	RequiredPaths.AddUnique(
		Route.ArrivalContainment.Mesh.ToSoftObjectPath());
	RequiredPaths.AddUnique(
		Route.ArrivalContainment.Material.ToSoftObjectPath());
	ArrivalContainmentLoadHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			RequiredPaths,
			FStreamableDelegate::CreateUObject(
				this,
				&ThisClass::HandleArrivalContainmentLoaded),
			FStreamableManager::AsyncLoadHighPriority);
	if (!ArrivalContainmentLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 도착 경고 원통의 비동기 로드를 시작하지 못했습니다."),
			*Route.RouteId.ToString());
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	HandleArrivalContainmentLoaded()
{
	LoadedArrivalContainmentMesh =
		Route.ArrivalContainment.Mesh.Get();
	LoadedArrivalContainmentMaterial =
		Route.ArrivalContainment.Material.Get();
	ArrivalContainmentLoadHandle.Reset();

	if (!LoadedArrivalContainmentMesh
		|| !LoadedArrivalContainmentMaterial
		|| !ArrivalContainmentPresentation)
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 도착 경고 원통 자산이 준비되지 않았습니다: Mesh=%s, Material=%s"),
			*Route.RouteId.ToString(),
			*Route.ArrivalContainment.Mesh.ToString(),
			*Route.ArrivalContainment.Material.ToString());
		return;
	}

	ArrivalContainmentPresentation->Configure(
		Route.ArrivalContainment,
		*LoadedArrivalContainmentMesh,
		*LoadedArrivalContainmentMaterial);
}

void ALastFPSStreamingLevelTransitionRuntime::
	CancelArrivalContainmentPreload()
{
	if (ArrivalContainmentLoadHandle.IsValid())
	{
		ArrivalContainmentLoadHandle->CancelHandle();
		ArrivalContainmentLoadHandle.Reset();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	HideArrivalContainment()
{
	GetWorldTimerManager().ClearTimer(
		ArrivalContainmentTimerHandle);
	if (ArrivalContainmentPresentation)
	{
		ArrivalContainmentPresentation->Hide();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	BeginDelayedEnemyContentPreload()
{
	CancelDelayedEnemyContentPreload();
	LoadedDelayedEnemyDefinition = nullptr;
	if (Route.DelayedEnemyDefinition.IsNull())
	{
		return;
	}

	TArray<FSoftObjectPath> RequiredPaths;
	RequiredPaths.AddUnique(
		Route.DelayedEnemyDefinition.ToSoftObjectPath());

	DelayedEnemyContentLoadHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			RequiredPaths,
			FStreamableDelegate::CreateUObject(
				this,
				&ThisClass::HandleDelayedEnemyContentLoaded),
			FStreamableManager::AsyncLoadHighPriority);
	if (!DelayedEnemyContentLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 정의의 비동기 로드를 시작하지 못했습니다: Definition=%s"),
			*Route.RouteId.ToString(),
			*Route.DelayedEnemyDefinition.ToString());
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	HandleDelayedEnemyContentLoaded()
{
	LoadedDelayedEnemyDefinition = Route.DelayedEnemyDefinition.Get();
	if (!LoadedDelayedEnemyDefinition)
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 정의의 비동기 로드 결과가 유효하지 않습니다: %s"),
			*Route.RouteId.ToString(),
			*Route.DelayedEnemyDefinition.ToString());
		return;
	}

	if (HasAuthority() && bDelayedEnemySpawnDue)
	{
		SpawnDelayedEnemy();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	CancelDelayedEnemyContentPreload()
{
	if (DelayedEnemyContentLoadHandle.IsValid())
	{
		DelayedEnemyContentLoadHandle->CancelHandle();
		DelayedEnemyContentLoadHandle.Reset();
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	RequestDestinationVisibility()
{
	if (!IsValid(DestinationStreamingLevel))
	{
		BeginDestinationPreload();
	}

	if (!IsValid(DestinationStreamingLevel))
	{
		return;
	}

	DestinationStreamingLevel->SetShouldBeLoaded(true);
	DestinationStreamingLevel->SetShouldBeVisible(true);
	if (DestinationStreamingLevel->IsLevelVisible() && HasAuthority())
	{
		CompletePendingPawnTransition();
	}
}

bool ALastFPSStreamingLevelTransitionRuntime::ResolveDestinationTransform(
	FTransform& OutTransform) const
{
	const ULevel* LoadedLevel = IsValid(DestinationStreamingLevel)
		? DestinationStreamingLevel->GetLoadedLevel()
		: nullptr;
	if (!LoadedLevel)
	{
		return false;
	}

	TArray<const APlayerStart*> Candidates;
	for (const AActor* Actor : LoadedLevel->Actors)
	{
		const APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
		if (!IsValid(PlayerStart)
			|| PlayerStart->IsA<APlayerStartPIE>())
		{
			continue;
		}

		if (Route.DestinationPlayerStartTag.IsNone()
			|| PlayerStart->ActorHasTag(
				Route.DestinationPlayerStartTag))
		{
			Candidates.Add(PlayerStart);
		}
	}

	if (Candidates.Num() != 1)
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 목적지 PlayerStart를 하나로 결정할 수 없습니다: Tag=%s, Candidates=%d"),
			*Route.RouteId.ToString(),
			*Route.DestinationPlayerStartTag.ToString(),
			Candidates.Num());
		return false;
	}

	OutTransform = Candidates[0]->GetActorTransform();
	OutTransform.AddToTranslation(Route.DestinationOffset);
	return true;
}

bool ALastFPSStreamingLevelTransitionRuntime::
	ResolveDelayedEnemySpawnTransform(FTransform& OutTransform) const
{
	const ULevel* LoadedLevel = IsValid(DestinationStreamingLevel)
		? DestinationStreamingLevel->GetLoadedLevel()
		: nullptr;
	if (!LoadedLevel)
	{
		return false;
	}

	TArray<const ATargetPoint*> Candidates;
	for (const AActor* Actor : LoadedLevel->Actors)
	{
		const ATargetPoint* TargetPoint = Cast<ATargetPoint>(Actor);
		if (!IsValid(TargetPoint))
		{
			continue;
		}

		if (Route.DelayedEnemySpawnPointTag.IsNone()
			|| TargetPoint->ActorHasTag(Route.DelayedEnemySpawnPointTag))
		{
			Candidates.Add(TargetPoint);
		}
	}

	if (Candidates.Num() != 1)
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 생성 TargetPoint를 하나로 결정할 수 없습니다: Tag=%s, Candidates=%d"),
			*Route.RouteId.ToString(),
			*Route.DelayedEnemySpawnPointTag.ToString(),
			Candidates.Num());
		return false;
	}

	OutTransform = Candidates[0]->GetActorTransform();
	OutTransform.AddToTranslation(Route.DelayedEnemySpawnOffset);
	return true;
}

void ALastFPSStreamingLevelTransitionRuntime::
	UpdateEncounterObjectiveMarker()
{
	if (Route.DelayedEnemyEncounterId.IsNone()
		|| !EncounterObjectiveMarker)
	{
		return;
	}

	FTransform SpawnTransform;
	if (!ResolveDelayedEnemySpawnTransform(SpawnTransform))
	{
		return;
	}

	EncounterObjectiveMarker->SetWorldTransform(SpawnTransform);
	if (bEncounterMarkerRegistered)
	{
		return;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World
		? World->GetGameInstance()
		: nullptr;
	if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance
		? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->RegisterEncounterMarker(
			Route.DelayedEnemyEncounterId,
			EncounterObjectiveMarker);
		bEncounterMarkerRegistered = true;
	}
}

void ALastFPSStreamingLevelTransitionRuntime::
	UnregisterEncounterObjectiveMarker()
{
	if (!bEncounterMarkerRegistered
		|| Route.DelayedEnemyEncounterId.IsNone()
		|| !EncounterObjectiveMarker)
	{
		return;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World
		? World->GetGameInstance()
		: nullptr;
	if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance
		? GameInstance->GetSubsystem<ULastFPSQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->UnregisterEncounterMarker(
			Route.DelayedEnemyEncounterId,
			EncounterObjectiveMarker);
	}
	bEncounterMarkerRegistered = false;
}

void ALastFPSStreamingLevelTransitionRuntime::
	CompletePendingPawnTransition()
{
	APawn* Pawn = PendingPawn.Get();
	if (!Pawn)
	{
		return;
	}

	FTransform DestinationTransform;
	if (!ResolveDestinationTransform(DestinationTransform))
	{
		return;
	}

	if (!Pawn->TeleportTo(
		DestinationTransform.GetLocation(),
		DestinationTransform.Rotator(),
		false,
		true))
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 플레이어 이동에 실패했습니다: Pawn=%s"),
			*Route.RouteId.ToString(),
			*GetNameSafe(Pawn));
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement =
			Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
	}
	if (AController* Controller = Pawn->GetController())
	{
		Controller->SetControlRotation(DestinationTransform.Rotator());
	}

	UE_LOG(
		LogLastFPSStreamingTransition,
		Log,
		TEXT("[%s] 플레이어를 스트리밍 목적지로 이동했습니다: Pawn=%s, Level=%s"),
		*Route.RouteId.ToString(),
		*GetNameSafe(Pawn),
		*Route.DestinationLevel.ToString());
	PendingPawn.Reset();
	const bool bArrivalHoldStarted =
		BeginArrivalHold(*Pawn);
	Multicast_NotifyTransitionArrived(
		DestinationTransform.GetLocation(),
		DestinationTransform.Rotator());
	if (!bArrivalHoldStarted)
	{
		ScheduleDelayedEnemySpawn();
	}
}

bool ALastFPSStreamingLevelTransitionRuntime::BeginArrivalHold(
	APawn& Pawn)
{
	const float HoldDuration = FMath::Max(
		Route.ArrivalHoldDuration,
		0.f);
	ACharacter* Character = Cast<ACharacter>(&Pawn);
	if (!Character || HoldDuration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement();
	if (!Movement)
	{
		return false;
	}

	ReleaseArrivalHold();
	HeldCharacter = Character;
	PreviousMovementMode = static_cast<uint8>(
		Movement->MovementMode.GetValue());
	PreviousCustomMovementMode = Movement->CustomMovementMode;
	Movement->StopMovementImmediately();
	Movement->DisableMovement();

	GetWorldTimerManager().SetTimer(
		ArrivalHoldTimerHandle,
		this,
		&ThisClass::HandleArrivalHoldFinished,
		HoldDuration,
		false);
	return true;
}

void ALastFPSStreamingLevelTransitionRuntime::
	HandleArrivalHoldFinished()
{
	ReleaseArrivalHold();
	ScheduleDelayedEnemySpawn();
}

void ALastFPSStreamingLevelTransitionRuntime::ReleaseArrivalHold()
{
	GetWorldTimerManager().ClearTimer(ArrivalHoldTimerHandle);

	ACharacter* Character = HeldCharacter.Get();
	HeldCharacter.Reset();
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement();
		Movement && Movement->MovementMode == MOVE_None)
	{
		Movement->SetMovementMode(
			static_cast<EMovementMode>(PreviousMovementMode),
			PreviousCustomMovementMode);
	}
}

void ALastFPSStreamingLevelTransitionRuntime::ScheduleDelayedEnemySpawn()
{
	if (!HasAuthority()
		|| bDelayedEnemySpawnScheduled
		|| Route.DelayedEnemyDefinition.IsNull())
	{
		return;
	}

	bDelayedEnemySpawnScheduled = true;
	const float SpawnDelay = FMath::Max(
		Route.DelayedEnemySpawnDelay,
		0.f);
	if (SpawnDelay <= KINDA_SMALL_NUMBER)
	{
		SpawnDelayedEnemy();
		return;
	}

	GetWorldTimerManager().SetTimer(
		DelayedEnemySpawnTimerHandle,
		this,
		&ThisClass::SpawnDelayedEnemy,
		SpawnDelay,
		false);

	UE_LOG(
		LogLastFPSStreamingTransition,
		Log,
		TEXT("[%s] 도착 이동 제한 해제 후 %.2f초 뒤 지연 적 생성을 예약했습니다."),
		*Route.RouteId.ToString(),
		SpawnDelay);
}

void ALastFPSStreamingLevelTransitionRuntime::SpawnDelayedEnemy()
{
	if (!HasAuthority() || SpawnedDelayedEnemy.IsValid())
	{
		return;
	}

	bDelayedEnemySpawnDue = true;
	if (!LoadedDelayedEnemyDefinition)
	{
		if (DelayedEnemyContentLoadHandle.IsValid()
			&& !DelayedEnemyContentLoadHandle->HasLoadCompleted())
		{
			UE_LOG(
				LogLastFPSStreamingTransition,
				Log,
				TEXT("[%s] 지연 적 생성 시점까지 콘텐츠 로드가 끝나지 않아 완료를 기다립니다."),
				*Route.RouteId.ToString());
			return;
		}

		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 정의가 준비되지 않아 생성할 수 없습니다: %s"),
			*Route.RouteId.ToString(),
			*Route.DelayedEnemyDefinition.ToString());
		return;
	}

	if (!LoadedDelayedEnemyDefinition->PawnClass
		|| !LoadedDelayedEnemyDefinition->PawnClass->IsChildOf(
			ALastFPSEnemyCharacter::StaticClass()))
	{
		UE_LOG(
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 정의의 PawnClass가 ALastFPSEnemyCharacter가 아닙니다: Definition=%s, PawnClass=%s"),
			*Route.RouteId.ToString(),
			*GetNameSafe(LoadedDelayedEnemyDefinition),
			*GetNameSafe(LoadedDelayedEnemyDefinition->PawnClass.Get()));
		return;
	}

	FTransform SpawnTransform;
	if (!ResolveDelayedEnemySpawnTransform(SpawnTransform))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* SpawnedPawn = nullptr;
	bool bAcquiredFromPool = false;
	if (ULastFPSActorPoolSubsystem* Pool =
		World->GetSubsystem<ULastFPSActorPoolSubsystem>())
	{
		SpawnedPawn = Cast<APawn>(Pool->AcquireActorByClass(
			LoadedDelayedEnemyDefinition->PawnClass,
			SpawnTransform,
			this,
			nullptr));
		bAcquiredFromPool = SpawnedPawn != nullptr;
	}
	if (!SpawnedPawn)
	{
		SpawnedPawn = World->SpawnActorDeferred<APawn>(
			LoadedDelayedEnemyDefinition->PawnClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	}

	ALastFPSEnemyCharacter* SpawnedEnemy =
		Cast<ALastFPSEnemyCharacter>(SpawnedPawn);
	if (!SpawnedEnemy)
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
			LogLastFPSStreamingTransition,
			Error,
			TEXT("[%s] 지연 적 액터 생성에 실패했습니다: Definition=%s"),
			*Route.RouteId.ToString(),
			*GetNameSafe(LoadedDelayedEnemyDefinition));
		return;
	}

	if (bAcquiredFromPool)
	{
		SpawnedEnemy->ResetForPoolReuse(LoadedDelayedEnemyDefinition);

		FVector AdjustedLocation = SpawnTransform.GetLocation();
		const FRotator AdjustedRotation = SpawnTransform.Rotator();

		if (UCapsuleComponent* Capsule = SpawnedEnemy->GetCapsuleComponent())
		{
			AdjustedLocation.Z += Capsule->GetScaledCapsuleHalfHeight();
		}

		if (World->FindTeleportSpot(
			SpawnedEnemy,
			AdjustedLocation,
			AdjustedRotation))
		{
			SpawnedEnemy->SetActorLocationAndRotation(
				AdjustedLocation,
				AdjustedRotation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
		else
		{
			// TeleportSpot을 찾지 못한 경우에도 최소한 바닥에 파묻히지 않도록 강제 적용한다.
			SpawnedEnemy->SetActorLocationAndRotation(
				AdjustedLocation,
				AdjustedRotation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		SpawnedEnemy->SetCharacterDefinitionForSpawn(
			LoadedDelayedEnemyDefinition);
		SpawnedEnemy->FinishSpawning(SpawnTransform);
	}

	SpawnedEnemy->SpawnDefaultController();
	SpawnedEnemy->OnDeath.AddUObject(
		this,
		&ThisClass::HandleDelayedEnemyDeath);
	if (Route.DelayedEnemySpawnGameplayCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC =
			SpawnedEnemy->GetAbilitySystemComponent())
		{
			FGameplayCueParameters CueParameters;
			ASC->ExecuteGameplayCue(
				Route.DelayedEnemySpawnGameplayCueTag,
				CueParameters);
		}
		else
		{
			UE_LOG(
				LogLastFPSStreamingTransition,
				Error,
				TEXT("[%s] 지연 적의 ASC를 찾을 수 없어 스폰 Gameplay Cue를 실행하지 못했습니다: Enemy=%s, Cue=%s"),
				*Route.RouteId.ToString(),
				*GetNameSafe(SpawnedEnemy),
				*Route.DelayedEnemySpawnGameplayCueTag.ToString());
		}
	}
	SpawnedDelayedEnemy = SpawnedEnemy;
	bDelayedEnemySpawnDue = false;

	Multicast_NotifyDelayedEnemyEncounterProgress(0, 1, false);

	UE_LOG(
		LogLastFPSStreamingTransition,
		Log,
		TEXT("[%s] 목적지에서 지연 적을 생성했습니다: Enemy=%s, Definition=%s, Pooled=%s"),
		*Route.RouteId.ToString(),
		*GetNameSafe(SpawnedEnemy),
		*GetNameSafe(LoadedDelayedEnemyDefinition),
		bAcquiredFromPool ? TEXT("true") : TEXT("false"));
}

void ALastFPSStreamingLevelTransitionRuntime::
	HandleDelayedEnemyDeath(ALastFPSCharacterBase* DeadCharacter)
{
	if (!HasAuthority()
		|| !DeadCharacter
		|| DeadCharacter != SpawnedDelayedEnemy.Get())
	{
		return;
	}

	DeadCharacter->OnDeath.RemoveAll(this);
	if (Route.DelayedEnemyEncounterId.IsNone())
	{
		return;
	}

	Multicast_NotifyDelayedEnemyEncounterProgress(1, 1, true);
}

FString ALastFPSStreamingLevelTransitionRuntime::
	MakeStreamingInstanceName() const
{
	return FString::Printf(
		TEXT("LastFPS_Transition_%s"),
		*Route.RouteId.ToString());
}
