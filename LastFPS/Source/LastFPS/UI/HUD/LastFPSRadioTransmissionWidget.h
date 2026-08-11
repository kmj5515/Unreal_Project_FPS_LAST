#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSRadioTransmissionWidget.generated.h"

class UTextBlock;
class UBorder;
class ULastFPSRadioAudioPlayer;

/**
 * 퍼스트 디센던트 스타일 HUD 무전 대사 자막 오버레이 위젯 C++ 베이스 클래스이다.
 * ULastFPSQuestSubsystem의 OnRadioTransmission 이벤트를 바인딩하여 
 * 노란색 화자 이름, 타이핑 대사 자막, 무전 음성 사운드 동기화 및 대기열(Queue) 재생을 담당한다.
 */
UCLASS()
class LASTFPS_API ULastFPSRadioTransmissionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 외부 또는 퀘스트 서브시스템에서 단일/연속 무전 대사를 요청할 때 호출한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Radio")
	void QueueRadioTransmission(const FLastFPSRadioTransmissionData& RadioData);

	/** 여러 건의 연속 무전 대사를 대기열에 순차 등록한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Radio")
	void QueueRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray);

protected:
	/** 블루프린트에서 바인딩할 위젯 컴포넌트들 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UI")
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UI")
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UI")
	TObjectPtr<UBorder> BackgroundPanel;

	/** 무전 대사가 시작/종료될 때 블루프린트 애니메이션(Fade-in / Fade-out)을 트리거할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Radio")
	void BP_OnRadioTransmissionStarted(const FLastFPSRadioTransmissionData& RadioData);

	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Radio")
	void BP_OnRadioTransmissionEnded();

private:
	UFUNCTION()
	void HandleQuestRadioTransmission(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray);

	void ProcessNextTransmission();
	void StartTypingNextChar();
	void FinishCurrentTransmission();

	TArray<FLastFPSRadioTransmissionData> TransmissionQueue;
	FLastFPSRadioTransmissionData CurrentTransmission;
	FString FullDialogueString;
	int32 CurrentCharIndex = 0;
	bool bIsPlaying = false;

	FTimerHandle TypingTimerHandle;
	FTimerHandle DisplayDurationTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSRadioAudioPlayer> RadioAudioPlayer;
};
