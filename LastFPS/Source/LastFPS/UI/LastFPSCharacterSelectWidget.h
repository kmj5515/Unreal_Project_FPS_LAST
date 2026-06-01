#pragma once

#include "UI/LastFPSActivatableWidget.h"
#include "LastFPSCharacterSelectWidget.generated.h"

class UButton;
class UTextBlock;
class ULastFPSCharacterCardWidget;

UCLASS()
class LASTFPS_API ULastFPSCharacterSelectWidget : public ULastFPSActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Prev;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Next;

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

	/** 에디터 Details에서 채우는 캐릭터 이름/역할 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterSelect")
	TArray<FText> CharacterNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|CharacterSelect")
	TArray<FText> CharacterRoles;

	/** C++ 기본 구현, Blueprint에서 오버라이드 가능 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void OnSelectionChanged(int32 NewIndex, int32 TotalCount);
	virtual void OnSelectionChanged_Implementation(int32 NewIndex, int32 TotalCount);

private:
	UFUNCTION() void HandleConfirmClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandlePrevClicked();
	UFUNCTION() void HandleNextClicked();
};
