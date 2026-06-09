#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSCharacterCardWidget.generated.h"

class ULastFPSCharacterDefinition;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnCardClicked, int32 /*CardIndex*/);

UCLASS()
class LASTFPS_API ULastFPSCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 이 카드에 표시할 캐릭터 정의 설정 — SelectWidget이 NativeConstruct에서 호출 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetupCard(const ULastFPSCharacterDefinition* Def);
	virtual void SetupCard_Implementation(const ULastFPSCharacterDefinition* Def);

	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetSelected(bool bSelected);
	virtual void SetSelected_Implementation(bool bSelected) {}

	/** 부모 SelectWidget이 NativeConstruct에서 지정 */
	int32 CardIndex = 0;

	FOnCardClicked OnCardClicked;

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CardName;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_CardRole;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
