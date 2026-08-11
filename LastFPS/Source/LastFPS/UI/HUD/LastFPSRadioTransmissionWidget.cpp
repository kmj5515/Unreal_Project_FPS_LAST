#include "UI/HUD/LastFPSRadioTransmissionWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestSubsystem.h"
#include "UI/HUD/Audio/LastFPSRadioAudioPlayer.h"
#include "UI/HUD/Audio/LastFPSRadioAudioSettings.h"

void ULastFPSRadioTransmissionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TransmissionQueue.Reset();
	bIsPlaying = false;

	RadioAudioPlayer = NewObject<ULastFPSRadioAudioPlayer>(this);
	RadioAudioPlayer->Initialize(this);

	SetVisibility(ESlateVisibility::Collapsed);
	if (BackgroundPanel)
	{
		BackgroundPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (ULastFPSQuestSubsystem* QuestSubsystem = GI->GetSubsystem<ULastFPSQuestSubsystem>())
			{
				QuestSubsystem->OnRadioTransmission.AddUniqueDynamic(
					this, &ULastFPSRadioTransmissionWidget::HandleQuestRadioTransmission);
				QuestSubsystem->FlushPendingRadioTransmissions();
			}
		}
	}
}

void ULastFPSRadioTransmissionWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (ULastFPSQuestSubsystem* QuestSubsystem = GI->GetSubsystem<ULastFPSQuestSubsystem>())
			{
				QuestSubsystem->OnRadioTransmission.RemoveDynamic(
					this, &ULastFPSRadioTransmissionWidget::HandleQuestRadioTransmission);
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TypingTimerHandle);
		World->GetTimerManager().ClearTimer(DisplayDurationTimerHandle);
	}

	if (RadioAudioPlayer)
	{
		RadioAudioPlayer->Stop();
		RadioAudioPlayer = nullptr;
	}

	TransmissionQueue.Reset();
	CurrentTransmission = FLastFPSRadioTransmissionData();
	FullDialogueString.Reset();
	CurrentCharIndex = 0;
	bIsPlaying = false;

	Super::NativeDestruct();
}

void ULastFPSRadioTransmissionWidget::HandleQuestRadioTransmission(
	const TArray<FLastFPSRadioTransmissionData>& RadioDataArray)
{
	QueueRadioTransmissions(RadioDataArray);
}

void ULastFPSRadioTransmissionWidget::QueueRadioTransmission(const FLastFPSRadioTransmissionData& RadioData)
{
	QueueRadioTransmissions({RadioData});
}

void ULastFPSRadioTransmissionWidget::QueueRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray)
{
	if (RadioDataArray.IsEmpty())
	{
		return;
	}

	// 최신 퀘스트/대화 요청이 도착하면 이전 상황의 재생 중 대사와 대기열을 폐기한다.
	// 같은 요청에 포함된 여러 대사는 배열 순서대로 계속 재생한다.
	TransmissionQueue = RadioDataArray;
	bIsPlaying = false;
	ProcessNextTransmission();
}

void ULastFPSRadioTransmissionWidget::ProcessNextTransmission()
{
	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DisplayDurationTimerHandle);

	if (TransmissionQueue.IsEmpty())
	{
		bIsPlaying = false;
		SetVisibility(ESlateVisibility::Collapsed);
		if (BackgroundPanel)
		{
			BackgroundPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		BP_OnRadioTransmissionEnded();
		return;
	}

	bIsPlaying = true;
	CurrentTransmission = TransmissionQueue[0];
	TransmissionQueue.RemoveAt(0);

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (BackgroundPanel)
	{
		BackgroundPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::SpeakerNameFormat),
			CurrentTransmission.SpeakerName));
		SpeakerNameText->SetColorAndOpacity(FSlateColor(CurrentTransmission.SpeakerColor));
	}

	FullDialogueString = CurrentTransmission.DialogueText.ToString();
	CurrentCharIndex = 0;

	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}

	if (RadioAudioPlayer)
	{
		RadioAudioPlayer->Stop();

		// 라디오 음성 적용을 다시 시작할 때 아래 호출을 복원한다.
		// RadioAudioPlayer->PlayTransmissionStart();
		// RadioAudioPlayer->PlayVoice(CurrentTransmission.VoiceAudio);
	}

	BP_OnRadioTransmissionStarted(CurrentTransmission);

	const ULastFPSRadioAudioSettings* RadioSettings = ULastFPSRadioAudioSettings::Get();
	const float TypingSpeedScale = RadioSettings ? RadioSettings->TypingSpeedScale : 1.0f;
	const float EffectiveTypingSpeed = CurrentTransmission.TypingSpeed * TypingSpeedScale;
	if (EffectiveTypingSpeed > KINDA_SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().SetTimer(
			TypingTimerHandle,
			this,
			&ULastFPSRadioTransmissionWidget::StartTypingNextChar,
			EffectiveTypingSpeed,
			true);
	}
	else
	{
		if (DialogueText)
		{
			DialogueText->SetText(FText::FromString(FullDialogueString));
		}
		const float Duration = CurrentTransmission.DisplayDuration > KINDA_SMALL_NUMBER ? CurrentTransmission.DisplayDuration : 4.0f;
		GetWorld()->GetTimerManager().SetTimer(
			DisplayDurationTimerHandle,
			this,
			&ULastFPSRadioTransmissionWidget::FinishCurrentTransmission,
			Duration,
			false);
	}
}

void ULastFPSRadioTransmissionWidget::StartTypingNextChar()
{
	CurrentCharIndex++;
	if (CurrentCharIndex > FullDialogueString.Len())
	{
		GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);

		const float Duration = CurrentTransmission.DisplayDuration > KINDA_SMALL_NUMBER ? CurrentTransmission.DisplayDuration : 4.0f;
		GetWorld()->GetTimerManager().SetTimer(
			DisplayDurationTimerHandle,
			this,
			&ULastFPSRadioTransmissionWidget::FinishCurrentTransmission,
			Duration,
			false);
		return;
	}

	if (DialogueText)
	{
		const FString SubStr = FullDialogueString.Left(CurrentCharIndex);
		DialogueText->SetText(FText::FromString(SubStr));
	}
}

void ULastFPSRadioTransmissionWidget::FinishCurrentTransmission()
{
	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DisplayDurationTimerHandle);

	// 라디오 음성 적용을 다시 시작할 때 종료 신호 호출도 함께 복원한다.
	// if (RadioAudioPlayer)
	// {
	// 	RadioAudioPlayer->PlayTransmissionEnd();
	// }

	ProcessNextTransmission();
}
