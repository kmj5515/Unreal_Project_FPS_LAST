#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "LastFPSGrapplingTargetingComponent.generated.h"

class ALastFPSHero;
class ULastFPSGrapplingHookData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnGrapplingTargetAvailabilityChanged,
	bool,
	bTargetAvailable);

/** 그래플링 어빌리티와 HUD가 동일한 타깃 유효성 규칙을 공유하도록 한다. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSGrapplingTargetingComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSGrapplingTargetingComponent();

	/** 어빌리티가 소유한 불변 설정을 미리보기 판정에도 사용하도록 등록한다. */
	void ConfigureTargeting(ULastFPSGrapplingHookData* InTargetingData);

	/** 캐시를 신뢰하지 않고 현재 카메라 방향으로 타깃을 다시 검증한다. */
	bool ResolveGrappleTarget(
		const ULastFPSGrapplingHookData& TargetingData,
		FHitResult& OutHit) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Grappling Targeting")
	bool IsTargetAvailable() const { return bTargetAvailable; }

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Grappling Targeting")
	FOnGrapplingTargetAvailabilityChanged OnTargetAvailabilityChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RestartPreviewTimer();

	UFUNCTION()
	void RefreshTargetAvailability();

	void SetTargetAvailability(bool bNewTargetAvailable);
	const ALastFPSHero* ResolveHero() const;

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSGrapplingHookData> TargetingData;

	FTimerHandle PreviewTimerHandle;
	bool bTargetAvailable = false;
};
