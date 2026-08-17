#include "Game/Streaming/LastFPSStreamingLevelTransitionSubsystem.h"

#include "Engine/TriggerBox.h"
#include "Engine/World.h"
#include "Game/Streaming/LastFPSStreamingLevelTransitionRuntime.h"
#include "Game/Streaming/LastFPSStreamingLevelTransitionSettings.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSStreamingTransitionSubsystem, Log, All);

bool ULastFPSStreamingLevelTransitionSubsystem::ShouldCreateSubsystem(
	UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World
		&& (World->WorldType == EWorldType::Game
			|| World->WorldType == EWorldType::PIE
			|| World->WorldType == EWorldType::GamePreview);
}

void ULastFPSStreamingLevelTransitionSubsystem::OnWorldBeginPlay(
	UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}

	const ULastFPSStreamingLevelTransitionSettings* Settings =
		GetDefault<ULastFPSStreamingLevelTransitionSettings>();
	if (!Settings)
	{
		return;
	}

	TArray<const FLastFPSStreamingLevelTransitionRoute*> Routes;
	Settings->GetRoutesForWorld(InWorld, Routes);
	if (Routes.IsEmpty())
	{
		return;
	}

	int32 InitializedRouteCount = 0;
	for (const FLastFPSStreamingLevelTransitionRoute* Route : Routes)
	{
		if (!Route)
		{
			continue;
		}

		FString FailureReason;
		if (!Route->IsValid(FailureReason))
		{
			UE_LOG(
				LogLastFPSStreamingTransitionSubsystem,
				Error,
				TEXT("스트리밍 전환 경로가 유효하지 않습니다: Route=%s, 원인=%s"),
				*Route->RouteId.ToString(),
				*FailureReason);
			continue;
		}

		if (!TryConfigureRoute(InWorld, *Route))
		{
			// 트리거가 아직 스트리밍되지 않았을 수 있다. 서브레벨이 들어오면 다시 시도한다.
			PendingRoutes.AddUnique(Route);
			continue;
		}

		++InitializedRouteCount;
	}

	if (!PendingRoutes.IsEmpty() && !LevelAddedHandle.IsValid())
	{
		LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
			this,
			&ULastFPSStreamingLevelTransitionSubsystem::HandleLevelAddedToWorld);
	}

	UE_LOG(
		LogLastFPSStreamingTransitionSubsystem,
		Log,
		TEXT("스트리밍 전환 경로 %d개를 구성했습니다. 트리거 대기 중: %d개"),
		InitializedRouteCount,
		PendingRoutes.Num());
}

void ULastFPSStreamingLevelTransitionSubsystem::HandleLevelAddedToWorld(
	ULevel* InLevel,
	UWorld* InWorld)
{
	UWorld* OwningWorld = GetWorld();
	if (!InLevel || !InWorld || InWorld != OwningWorld || PendingRoutes.IsEmpty())
	{
		return;
	}

	for (int32 Index = PendingRoutes.Num() - 1; Index >= 0; --Index)
	{
		const FLastFPSStreamingLevelTransitionRoute* Route = PendingRoutes[Index];
		if (!Route || TryConfigureRoute(*OwningWorld, *Route))
		{
			PendingRoutes.RemoveAt(Index);
		}
	}

	if (PendingRoutes.IsEmpty())
	{
		StopWaitingForPendingRoutes();
	}
}

void ULastFPSStreamingLevelTransitionSubsystem::StopWaitingForPendingRoutes()
{
	if (LevelAddedHandle.IsValid())
	{
		FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
		LevelAddedHandle.Reset();
	}
}

void ULastFPSStreamingLevelTransitionSubsystem::Deinitialize()
{
	StopWaitingForPendingRoutes();
	PendingRoutes.Reset();
	Super::Deinitialize();
}

bool ULastFPSStreamingLevelTransitionSubsystem::TryConfigureRoute(
	UWorld& InWorld,
	const FLastFPSStreamingLevelTransitionRoute& Route)
{
	// 스트리밍으로 새 레벨이 들어올 때마다 다시 훑어야 하므로 여기서 수집한다.
	TArray<AActor*> TriggerActors;
	UGameplayStatics::GetAllActorsOfClass(
		&InWorld,
		ATriggerBox::StaticClass(),
		TriggerActors);

	ATriggerBox* TriggerVolume = nullptr;
	for (AActor* TriggerActor : TriggerActors)
	{
		if (TriggerActor
			&& TriggerActor->ActorHasTag(Route.TriggerMarkerTag)
			&& TriggerActor->ActorHasTag(Route.TriggerRouteTag))
		{
			TriggerVolume = Cast<ATriggerBox>(TriggerActor);
			break;
		}
	}

	if (!TriggerVolume)
	{
		UE_LOG(
			LogLastFPSStreamingTransitionSubsystem,
			Warning,
			TEXT("[%s] 전환 TriggerBox가 아직 없습니다. 서브레벨 스트리밍을 기다립니다: Marker=%s, RouteTag=%s"),
			*Route.RouteId.ToString(),
			*Route.TriggerMarkerTag.ToString(),
			*Route.TriggerRouteTag.ToString());
		return false;
	}

	const FName RuntimeName = MakeUniqueObjectName(
		&InWorld,
		ALastFPSStreamingLevelTransitionRuntime::StaticClass(),
		FName(*FString::Printf(
			TEXT("StreamingTransitionRuntime_%s"),
			*Route.RouteId.ToString())));
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = RuntimeName;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.bDeferConstruction = true;

	ALastFPSStreamingLevelTransitionRuntime* Runtime =
		InWorld.SpawnActor<ALastFPSStreamingLevelTransitionRuntime>(
			ALastFPSStreamingLevelTransitionRuntime::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
	if (!Runtime)
	{
		UE_LOG(
			LogLastFPSStreamingTransitionSubsystem,
			Error,
			TEXT("[%s] 스트리밍 전환 런타임을 생성하지 못했습니다."),
			*Route.RouteId.ToString());
		return false;
	}

	Runtime->ConfigureRoute(Route, *TriggerVolume);
	Runtime->FinishSpawning(FTransform::Identity);

	UE_LOG(
		LogLastFPSStreamingTransitionSubsystem,
		Log,
		TEXT("[%s] 스트리밍 전환 경로를 구성했습니다: Trigger=%s"),
		*Route.RouteId.ToString(),
		*GetNameSafe(TriggerVolume));
	return true;
}
