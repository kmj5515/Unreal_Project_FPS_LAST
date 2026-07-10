#pragma once

#include "UI/Framework/LastFPSActivatableWidget.h"
#include "LastFPSCharacterSelectWidget.generated.h"

class ULastFPSButtonBase;
class UTextBlock;
class ULastFPSCharacterCardWidget;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterRoster;

UCLASS()
class LASTFPS_API ULastFPSCharacterSelectWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSButtonBase> Button_Back;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSCharacterCardWidget> Card_0;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSCharacterCardWidget> Card_1;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<ULastFPSCharacterCardWidget> Card_2;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CharName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CharRole;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CharDesc;

	/** C++ 기본 구현, Blueprint에서 오버라이드 가능 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void OnSelectionChanged(int32 NewIndex, int32 TotalCount);
	virtual void OnSelectionChanged_Implementation(int32 NewIndex, int32 TotalCount);

private:
	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleBackClicked();

	void HandleCardClicked(int32 Index);

	/** 카드 선택 표시(하이라이트) 갱신 — BP 오버라이드 영향 없이 항상 C++에서 구동 */
	void UpdateCardSelection(int32 SelectedIndex);

	/** GameInstance의 캐릭터 로스터(단일 소스). 없으면 nullptr. */
	const ULastFPSCharacterRoster* GetCharacterRoster() const;

};
