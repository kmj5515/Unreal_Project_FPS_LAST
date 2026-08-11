#include "Cinematics/LastFPSCinematicPlaybackSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSCinematic, Log, All);

ULastFPSCinematicPlaybackSubsystem* ULastFPSCinematicPlaybackSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	return World ? World->GetSubsystem<ULastFPSCinematicPlaybackSubsystem>() : nullptr;
}

bool ULastFPSCinematicPlaybackSubsystem::RequestPlayback(const FLastFPSCinematicPlayback& Request)
{
	if (!Request.IsValidRequest())
	{
		return false; // 연출 없는 훅 — 정상 흐름이라 로그를 남기지 않는다.
	}

	if (!MatchesRequiredWorld(Request))
	{
		return false; // 다른 맵용 컷신 — 바인딩이 성립하지 않으므로 조용히 생략한다.
	}

	// 컷신은 화면 전체를 점유하므로 겹쳐 재생하지 않는다. 늦게 온 요청을 버리고 원인을 남긴다.
	if (bSlotOccupied)
	{
		UE_LOG(
			LogLastFPSCinematic,
			Warning,
			TEXT("컷신 '%s' 재생 요청을 거부했습니다 — 이미 다른 컷신이 재생 중입니다."),
			*Request.Sequence.ToSoftObjectPath().ToString());
		return false;
	}

	bSlotOccupied = true;
	PendingRequest = Request;
	ScheduleLoad();
	return true;
}

bool ULastFPSCinematicPlaybackSubsystem::MatchesRequiredWorld(const FLastFPSCinematicPlayback& Request) const
{
	if (Request.RequiredWorld.IsNull())
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World || !World->GetOutermost())
	{
		return false;
	}

	// PIE 접두사를 제거한 영속 월드 경로로 비교해 에디터 재생에서도 같은 판정을 유지한다.
	const FString CurrentPackageName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	const FString RequiredPackageName = Request.RequiredWorld.ToSoftObjectPath().GetLongPackageName();
	return !RequiredPackageName.IsEmpty() && RequiredPackageName == CurrentPackageName;
}

void ULastFPSCinematicPlaybackSubsystem::ScheduleLoad()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishPlayback(false);
		return;
	}

	if (PendingRequest.StartDelay <= 0.f)
	{
		BeginLoad();
		return;
	}

	World->GetTimerManager().SetTimer(
		StartDelayTimerHandle,
		FTimerDelegate::CreateUObject(this, &ULastFPSCinematicPlaybackSubsystem::BeginLoad),
		PendingRequest.StartDelay,
		false);
}

void ULastFPSCinematicPlaybackSubsystem::BeginLoad()
{
	// 이미 메모리에 있으면 한 프레임도 미루지 않고 바로 재생한다.
	if (PendingRequest.Sequence.Get())
	{
		HandleSequenceLoaded();
		return;
	}

	SequenceLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PendingRequest.Sequence.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ULastFPSCinematicPlaybackSubsystem::HandleSequenceLoaded),
		FStreamableManager::AsyncLoadHighPriority);

	if (!SequenceLoadHandle.IsValid())
	{
		UE_LOG(
			LogLastFPSCinematic,
			Error,
			TEXT("컷신 '%s' 비동기 로드 요청을 시작하지 못했습니다."),
			*PendingRequest.Sequence.ToSoftObjectPath().ToString());
		FinishPlayback(false);
	}
}

void ULastFPSCinematicPlaybackSubsystem::HandleSequenceLoaded()
{
	SequenceLoadHandle.Reset();

	ULevelSequence* Sequence = PendingRequest.Sequence.Get();
	if (!Sequence)
	{
		UE_LOG(
			LogLastFPSCinematic,
			Error,
			TEXT("컷신 '%s' 로드에 실패해 재생을 취소합니다."),
			*PendingRequest.Sequence.ToSoftObjectPath().ToString());
		FinishPlayback(false);
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false; // 재생 시점은 이 서브시스템이 소유한다.
	Settings.PlayRate = PendingRequest.PlayRate;
	Settings.bHideHud = PendingRequest.bHideHUD;
	Settings.bHidePlayer = PendingRequest.bHidePlayer;
	Settings.bDisableMovementInput = PendingRequest.bDisableMovementInput;
	Settings.bDisableLookAtInput = PendingRequest.bDisableLookAtInput;
	// 컷신은 연출 전 상태로 되돌아가는 것이 기본이다. 스킵과 정상 종료가 같은 상태로 끝나야
	// 이후 게임플레이가 재생 여부에 따라 갈리지 않는다.
	Settings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;

	ALevelSequenceActor* CreatedActor = nullptr;
	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Sequence, Settings, CreatedActor);
	SequenceActor = CreatedActor;

	if (!SequencePlayer || !SequenceActor)
	{
		UE_LOG(
			LogLastFPSCinematic,
			Error,
			TEXT("컷신 '%s' 의 시퀀스 플레이어를 만들지 못했습니다."),
			*Sequence->GetPathName());
		FinishPlayback(false);
		return;
	}

	SequencePlayer->OnFinished.AddDynamic(this, &ULastFPSCinematicPlaybackSubsystem::HandlePlayerFinished);
	SequencePlayer->Play();

	OnCinematicStarted.Broadcast(PendingRequest.bSkippable);
}

bool ULastFPSCinematicPlaybackSubsystem::TrySkip()
{
	if (!IsPlaying() || !PendingRequest.bSkippable)
	{
		return false;
	}

	SequencePlayer->Stop();
	FinishPlayback(true);
	return true;
}

void ULastFPSCinematicPlaybackSubsystem::StopPlayback()
{
	if (!bSlotOccupied)
	{
		return;
	}

	if (SequencePlayer)
	{
		SequencePlayer->Stop();
	}
	FinishPlayback(false);
}

bool ULastFPSCinematicPlaybackSubsystem::IsPlaying() const
{
	return bSlotOccupied && SequencePlayer != nullptr;
}

void ULastFPSCinematicPlaybackSubsystem::HandlePlayerFinished()
{
	FinishPlayback(false);
}

void ULastFPSCinematicPlaybackSubsystem::FinishPlayback(const bool bSkipped)
{
	// 스킵(Stop → 직접 호출)과 정상 종료(OnFinished)가 겹쳐 들어올 수 있어 한 번만 처리한다.
	if (!bSlotOccupied)
	{
		return;
	}
	bSlotOccupied = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartDelayTimerHandle);
	}

	if (SequenceLoadHandle.IsValid())
	{
		SequenceLoadHandle->CancelHandle();
		SequenceLoadHandle.Reset();
	}

	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &ULastFPSCinematicPlaybackSubsystem::HandlePlayerFinished);
		SequencePlayer = nullptr;
	}

	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}

	PendingRequest = FLastFPSCinematicPlayback();

	OnCinematicFinished.Broadcast(bSkipped);
}

void ULastFPSCinematicPlaybackSubsystem::Deinitialize()
{
	StopPlayback();
	Super::Deinitialize();
}
