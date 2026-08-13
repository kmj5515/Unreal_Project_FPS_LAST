#include "Cinematics/LastFPSCinematicPlaybackSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "Data/AssetManagement/LastFPSGameDataSubsystem.h"
#include "Data/Tables/LastFPSDialogueData.h"
#include "TimerManager.h"
#include "Data/AssetManagement/LastFPSGameDataTags.h"
#include "UObject/Package.h"
#include "HAL/IConsoleManager.h"

#if WITH_EDITOR
//
#endif

static FAutoConsoleCommand CVarLastFPSAddMarks(
	TEXT("LastFPS.AddMarksForRadios"),
	TEXT("Adds radio marks to sequences"),
	FConsoleCommandDelegate::CreateStatic([]()
	{
#if WITH_EDITOR
		auto ReplaceMarks = [](const FString& Path, const TArray<TPair<FString, int32>>& NewMarks)
		{
			if (UObject* Obj = LoadObject<UMovieSceneSequence>(nullptr, *Path))
			{
				if (UMovieSceneSequence* Seq = Cast<UMovieSceneSequence>(Obj))
				{
					if (UMovieScene* MS = Seq->GetMovieScene())
					{
						MS->Modify();
						MS->DeleteMarkedFrames();
						
						FFrameRate DisplayRate = MS->GetDisplayRate();
						FFrameRate TickResolution = MS->GetTickResolution();
						
						for (const auto& Pair : NewMarks)
						{
							FMovieSceneMarkedFrame NewMark;
							NewMark.FrameNumber = FFrameRate::TransformTime(FFrameTime(Pair.Value), DisplayRate, TickResolution).FrameNumber;
							NewMark.Label = Pair.Key;
							MS->AddMarkedFrame(NewMark);
							UE_LOG(LogTemp, Warning, TEXT("MarkedFrame added: %s at DisplayFrame %d"), *Pair.Key, Pair.Value);
						}
						
						Seq->MarkPackageDirty();
					}
				}
			}
		};

		TArray<TPair<FString, int32>> HuntMarks = {
			{TEXT("Dialogue:DLG_Hunt_Start_1"), 0},
			{TEXT("Dialogue:DLG_Hunt_Start_2"), 57},
			{TEXT("Dialogue:DLG_Hunt_Start_3"), 111}
		};
		ReplaceMarks(TEXT("/Game/Cinematics/Quest/Quest_Hunt/LS_Quest_Hunt_Start.LS_Quest_Hunt_Start"), HuntMarks);

		TArray<TPair<FString, int32>> MobMarks = {
			{TEXT("Dialogue:DLG_Mobility_Start_1"), 0},
			{TEXT("Dialogue:DLG_Mobility_Start_2"), 54},
			{TEXT("Dialogue:DLG_Mobility_Start_3"), 105}
		};
		ReplaceMarks(TEXT("/Game/Cinematics/Quest/Q_Mobility/LS_Q_Mobility_Start.LS_Q_Mobility_Start"), MobMarks);
		UE_LOG(LogTemp, Warning, TEXT("Dialogues added to sequences!"));
#endif
	})
);

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
	
	// Initialize LastFrameNumber for mark tracking (subtract 1 so we catch markers at the very start frame)
	LastFrameNumber = SequencePlayer->GetStartTime().Time.FrameNumber - 1;
	
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

	// 시퀀스 재생이 끝났거나 스킵되었을 때 진행 중이던 라디오도 모두 종료한다.
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<ULastFPSQuestSubsystem>())
			{
				QuestSubsystem->StopAllRadioTransmissions();
			}
		}
	}

	OnCinematicFinished.Broadcast(bSkipped);
}

void ULastFPSCinematicPlaybackSubsystem::Deinitialize()
{
	StopPlayback();
	Super::Deinitialize();
}

void ULastFPSCinematicPlaybackSubsystem::Tick(float DeltaTime)
{
	if (IsPlaying() && SequencePlayer)
	{
		FQualifiedFrameTime CurrentTime = SequencePlayer->GetCurrentTime();
		FFrameNumber CurrentFrame = CurrentTime.Time.FrameNumber;
		
		if (CurrentFrame != LastFrameNumber)
		{
			if (UMovieSceneSequence* Sequence = SequencePlayer->GetSequence())
			{
				if (UMovieScene* MovieScene = Sequence->GetMovieScene())
				{
					FFrameRate PlayerRate = CurrentTime.Rate;
					FFrameRate TickResolution = MovieScene->GetTickResolution();
					
					const TArray<FMovieSceneMarkedFrame>& MarkedFrames = MovieScene->GetMarkedFrames();
					for (const FMovieSceneMarkedFrame& Mark : MarkedFrames)
					{
						// Convert MarkFrame from TickResolution to PlayerRate
						FFrameNumber MarkFrame = FFrameRate::TransformTime(FFrameTime(Mark.FrameNumber), TickResolution, PlayerRate).FrameNumber;
						
						bool bPassedForward = (LastFrameNumber < MarkFrame && CurrentFrame >= MarkFrame);
						bool bPassedBackward = (LastFrameNumber > MarkFrame && CurrentFrame <= MarkFrame);
						
						if (bPassedForward || bPassedBackward)
						{
							FString LabelStr = Mark.Label;
							if (LabelStr.StartsWith(TEXT("Radio:")))
							{
								FName RadioId = FName(*LabelStr.Mid(6));
								if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
								{
									if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<ULastFPSQuestSubsystem>())
									{
										QuestSubsystem->TriggerRadioByIds({RadioId});
									}
								}
							}
							else if (LabelStr.StartsWith(TEXT("Dialogue:")))
							{
								FName DialogueId = FName(*LabelStr.Mid(9));
								if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
								{
									if (ULastFPSQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<ULastFPSQuestSubsystem>())
									{
										if (ULastFPSGameDataSubsystem* GameData = GameInstance->GetSubsystem<ULastFPSGameDataSubsystem>())
										{
											if (UDataTable* DT = GameData->FindTable(LastFPSGameDataTags::Data_Table_NPC_Dialogue))
											{
												if (FLastFPSDialogueData* DialogueRow = DT->FindRow<FLastFPSDialogueData>(DialogueId, TEXT("CinematicDialogue")))
												{
													QuestSubsystem->TriggerDialogueAsRadio(*DialogueRow, FText::GetEmpty(), FLinearColor::White);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			LastFrameNumber = CurrentFrame;
		}
	}
}

TStatId ULastFPSCinematicPlaybackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULastFPSCinematicPlaybackSubsystem, STATGROUP_Tickables);
}
