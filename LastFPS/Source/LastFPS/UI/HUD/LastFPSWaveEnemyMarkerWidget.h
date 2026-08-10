#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSWaveEnemyMarkerWidget.generated.h"

class UTextBlock;

/** 웨이브의 마지막 남은 적을 알리는 머리 위 빨간 화살표다. */
UCLASS()
class LASTFPS_API ULastFPSWaveEnemyMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	void EnsureNativeWidget();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ArrowText;
};
