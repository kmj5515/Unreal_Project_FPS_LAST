#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSCharacterCardWidget.generated.h"

UCLASS()
class LASTFPS_API ULastFPSCharacterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|UI")
	void SetSelected(bool bSelected);
	virtual void SetSelected_Implementation(bool bSelected) {}
};
