#pragma once

#include "UObject/Interface.h"
#include "ILastFPSInteractable.generated.h"

class APlayerController;

UINTERFACE(MinimalAPI, Blueprintable)
class ULastFPSInteractable : public UInterface
{
	GENERATED_BODY()
};

class LASTFPS_API ILastFPSInteractable
{
	GENERATED_BODY()

public:
	/** 플레이어가 F 키를 눌렀을 때 호출 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Interaction")
	void Interact(APlayerController* InstigatorPC);

	/** 마커에 표시할 행동 텍스트 (예: "대화", "상점") */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Interaction")
	FText GetInteractionLabel() const;
};
