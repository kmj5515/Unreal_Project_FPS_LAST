#pragma once

#include "UI/Framework/LastFPSButtonBase.h"
#include "Utility/LastFPSTravelTypes.h"
#include "LastFPSTravelEntryButton.generated.h"

class UTexture2D;
class UImage;

/**
 * 이동 목적지만 보관하고 실제 이동 수행은 소유 화면과 이동 서브시스템에 위임한다.
 */
UCLASS(Abstract, Blueprintable)
class LASTFPS_API ULastFPSTravelEntryButton : public ULastFPSButtonBase
{
	GENERATED_BODY()

public:
	const FLastFPSTravelEntryRequest& GetTravelRequest() const
	{
		return TravelRequest;
	}

	/** 화면이 해석한 접근 상태만 받아 버튼 동작과 잠금 표시를 함께 갱신한다. */
	void ApplyTravelAccess(bool bUnlocked, TSoftObjectPtr<UTexture2D> LockedIcon);

protected:
	virtual void NativePreConstruct() override;

	/** 자동 오버레이 이후 프로젝트별 추가 연출이 필요할 때 쓰는 Blueprint 확장 훅이다. */
	UFUNCTION(BlueprintImplementableEvent, Category="LastFPS|Travel")
	void OnTravelLockStateChanged(bool bIsTravelLocked, const TSoftObjectPtr<UTexture2D>& LockedIcon);

	/** 버튼 중앙에 표시할 잠금 아이콘 크기다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Travel", meta=(ClampMin="8.0"))
	float LockedIconSize = 48.f;

private:
	void EnsureLockedVisual();
	void RefreshLockedVisual();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Travel",
		meta=(AllowPrivateAccess="true"))
	FLastFPSTravelEntryRequest TravelRequest;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UImage> LockedImage;

	TSoftObjectPtr<UTexture2D> LockedIconReference;
	bool bTravelLocked = false;
};
