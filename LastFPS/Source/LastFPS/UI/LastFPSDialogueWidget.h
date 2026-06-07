#pragma once

#include "UI/LastFPSModalDialogBase.h"
#include "LastFPSDialogueWidget.generated.h"

class ULastFPSButtonBase;
class UTextBlock;

/**
 * 단방향 NPC 대화창 (Modal 레이어).
 * 화자 이름(Text_Title) + 현재 대사(Text_Body)는 베이스에서 가져온다.
 * Button_Next: 다음 페이지로 진행, 마지막 페이지에서는 닫기.
 */
UCLASS()
class LASTFPS_API ULastFPSDialogueWidget : public ULastFPSModalDialogBase
{
	GENERATED_BODY()

public:
	/** 화자 + 대사 목록으로 대화 시작. Lines가 비면 즉시 닫힘. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void SetupDialogue(const FText& InSpeaker, const TArray<FText>& InLines);

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnHandleBackAction() override;

	/** 다음/닫기 버튼 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Next;

	/** 버튼 라벨 ("다음" ↔ "닫기" 토글, 선택) */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_NextLabel;

	/** "다음" 라벨 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|UI")
	FText NextLabel = NSLOCTEXT("LastFPS", "Dialogue_Next", "다음");

	/** 마지막 페이지에서의 라벨 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|UI")
	FText CloseLabel = NSLOCTEXT("LastFPS", "Dialogue_Close", "닫기");

private:
	UFUNCTION()
	void HandleNextClicked();

	/** 현재 인덱스의 대사를 표시하고 버튼 라벨을 갱신 */
	void ShowCurrentLine();

	void CloseDialogue();

	UPROPERTY(Transient)
	FText Speaker;

	UPROPERTY(Transient)
	TArray<FText> Lines;

	int32 CurrentIndex = 0;
};
