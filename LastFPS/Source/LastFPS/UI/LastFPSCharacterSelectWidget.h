#pragma once

#include "UI/LastFPSActivatableWidget.h"
#include "LastFPSCharacterSelectWidget.generated.h"

class ULastFPSButtonBase;
class UTextBlock;
class ULastFPSCharacterCardWidget;
class ULastFPSCharacterDefinition;

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

	/** 카드별 캐릭터 정의(DataAsset) 목록 — 이름/역할은 여기서 읽는다.
	 *  PlayerController의 SelectableCharacterClasses와 인덱스 순서를 맞춰 둘 것. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterSelect")
	TArray<TObjectPtr<ULastFPSCharacterDefinition>> CharacterDefinitions;

	/** C++ 기본 구현, Blueprint에서 오버라이드 가능 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void OnSelectionChanged(int32 NewIndex, int32 TotalCount);
	virtual void OnSelectionChanged_Implementation(int32 NewIndex, int32 TotalCount);

private:
	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleBackClicked();

	void HandleCardClicked(int32 Index);
};
