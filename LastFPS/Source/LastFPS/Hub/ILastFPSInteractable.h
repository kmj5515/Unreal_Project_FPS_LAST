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
	/** 홀드 게이지가 가득 찼을 때 호출 (G 키를 일정 시간 누르면 발동) */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Interaction")
	void Interact(APlayerController* InstigatorPC);

	/** 마커에 표시할 행동 텍스트 (예: "대화", "상점") */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Interaction")
	FText GetInteractionLabel() const;

	/**
	 * 홀드 진행도(0~1) 갱신 — 원형 게이지 등 피드백용.
	 * 0이면 게이지 숨김/리셋. 피드백이 필요 없는 액터는 구현하지 않으면 된다(기본 무동작).
	 */
	UFUNCTION(BlueprintNativeEvent, Category="LastFPS|Interaction")
	void SetInteractionProgress(float Progress);
};
