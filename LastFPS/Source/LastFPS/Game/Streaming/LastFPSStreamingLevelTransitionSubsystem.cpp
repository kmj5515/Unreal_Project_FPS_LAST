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

	TArray<AActor*> TriggerActors;
	UGameplayStatics::GetAllActorsOfClass(
		&InWorld,
		ATriggerBox::StaticClass(),
		TriggerActors);

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

		ATriggerBox* TriggerVolume = nullptr;
		for (AActor* TriggerActor : TriggerActors)
		{
			if (TriggerActor
				&& TriggerActor->ActorHasTag(Route->TriggerMarkerTag)
				&& TriggerActor->ActorHasTag(Route->TriggerRouteTag))
			{
				TriggerVolume = Cast<ATriggerBox>(TriggerActor);
				break;
			}
		}

		if (!TriggerVolume)
		{
			UE_LOG(
				LogLastFPSStreamingTransitionSubsystem,
				Error,
				TEXT("[%s] 전환 TriggerBox를 찾을 수 없습니다: Marker=%s, RouteTag=%s"),
				*Route->RouteId.ToString(),
				*Route->TriggerMarkerTag.ToString(),
				*Route->TriggerRouteTag.ToString());
			continue;
		}

		const FName RuntimeName = MakeUniqueObjectName(
			&InWorld,
			ALastFPSStreamingLevelTransitionRuntime::StaticClass(),
			FName(*FString::Printf(
				TEXT("StreamingTransitionRuntime_%s"),
				*Route->RouteId.ToString())));
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
				*Route->RouteId.ToString());
			continue;
		}

		Runtime->ConfigureRoute(*Route, *TriggerVolume);
		Runtime->FinishSpawning(FTransform::Identity);
		++InitializedRouteCount;
	}

	UE_LOG(
		LogLastFPSStreamingTransitionSubsystem,
		Log,
		TEXT("스트리밍 레벨 전환 경로 %d개를 초기화했습니다: World=%s"),
		InitializedRouteCount,
		*InWorld.GetName());
}
