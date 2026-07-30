#include "UI/HUD/LastFPSRadioTransmissionWidget.h"

#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Quest/LastFPSQuestSubsystem.h"

void ULastFPSRadioTransmissionWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DisplayDurationTimerHandle);

	if (ActiveVoiceAudioComponent && ActiveVoiceAudioComponent->IsPlaying())
	{
		ActiveVoiceAudioComponent->Stop();
	}

	Super::NativeDestruct();
}

void ULastFPSRadioTransmissionWidget::HandleQuestRadioTransmission(const FLastFPSRadioTransmissionData& RadioData)
{
	QueueRadioTransmission(RadioData);
}

void ULastFPSRadioTransmissionWidget::QueueRadioTransmission(const FLastFPSRadioTransmissionData& RadioData)
{
	TransmissionQueue.Add(RadioData);
	if (!bIsPlaying)
	{
		ProcessNextTransmission();
	}
}

void ULastFPSRadioTransmissionWidget::QueueRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray)
{
	for (const FLastFPSRadioTransmissionData& Data : RadioDataArray)
	{
		TransmissionQueue.Add(Data);
	}
	if (!bIsPlaying)
	{
		ProcessNextTransmission();
	}
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
		const FString FormattedSpeaker = FString::Printf(TEXT("%s:"), *CurrentTransmission.SpeakerName.ToString());
		SpeakerNameText->SetText(FText::FromString(FormattedSpeaker));
		SpeakerNameText->SetColorAndOpacity(FSlateColor(CurrentTransmission.SpeakerColor));
	}

	FullDialogueString = CurrentTransmission.DialogueText.ToString();
	CurrentCharIndex = 0;

	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}

	// 음성 사운드 재생
	if (ActiveVoiceAudioComponent && ActiveVoiceAudioComponent->IsPlaying())
	{
		ActiveVoiceAudioComponent->Stop();
		ActiveVoiceAudioComponent = nullptr;
	}

	if (!CurrentTransmission.VoiceAudio.IsNull())
	{
		if (USoundBase* LoadedSound = CurrentTransmission.VoiceAudio.LoadSynchronous())
		{
			ActiveVoiceAudioComponent = UGameplayStatics::SpawnSound2D(this, LoadedSound);
		}
	}

	BP_OnRadioTransmissionStarted(CurrentTransmission);

	if (CurrentTransmission.TypingSpeed > KINDA_SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().SetTimer(
			TypingTimerHandle,
			this,
			&ULastFPSRadioTransmissionWidget::StartTypingNextChar,
			CurrentTransmission.TypingSpeed,
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

	ProcessNextTransmission();
}
