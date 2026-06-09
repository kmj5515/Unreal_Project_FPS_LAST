#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSCharacterCardWidget.generated.h"

DECLARE_DELEGATE_OneParam(FOnCardClicked, int32 /*CardIndex*/);

UCLASS()
class LASTFPS_API ULastFPSCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetSelected(bool bSelected);
	virtual void SetSelected_Implementation(bool bSelected) {}

	/** 부모 SelectWidget이 NativeConstruct에서 지정 */
	int32 CardIndex = 0;

	FOnCardClicked OnCardClicked;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
