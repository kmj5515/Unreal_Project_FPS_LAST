#pragma once

#include "GameUIManagerSubsystem.h"
#include "GameplayTagContainer.h"
#include "LastFPSUIManagerSubsystem.generated.h"

class UCommonActivatableWidget;
class ULastFPSScreenRegistry;
class APlayerController;

/**
 * LastFPS UI manager — UGameUIPolicy로 로컬 플레이어별 PrimaryGameLayout 생성 +
 * 태그 기반 화면 라우팅(OpenScreen) 제공.
 *
 * 콘텐츠 추가 = ScreenRegistry에 행 1개. 호출부(NPC/버튼/GameMode)는 OpenScreen(Tag)만.
 */
UCLASS()
class LASTFPS_API ULastFPSUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** WorldContext로 인스턴스 획득 (BP/C++ 공용) */
	static ULastFPSUIManagerSubsystem* Get(const UObject* WorldContext);

	/**
	 * 화면 태그에 해당하는 위젯을 레지스트리에서 찾아 해당 레이어에 push.
	 * 이미 열려 있으면 기존 위젯 반환. OwningPlayer 미지정 시 첫 로컬 PC 사용.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	UCommonActivatableWidget* OpenScreen(FGameplayTag ScreenTag, APlayerController* OwningPlayer = nullptr);

	/** 열려 있던 화면을 닫는다(pop). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|UI")
	void CloseScreen(FGameplayTag ScreenTag);

	UFUNCTION(BlueprintPure, Category="LastFPS|UI")
	bool IsScreenOpen(FGameplayTag ScreenTag) const;

private:
	const ULastFPSScreenRegistry* GetRegistry();

	UPROPERTY(Transient)
	TObjectPtr<ULastFPSScreenRegistry> CachedRegistry;

	/** 현재 열려 있는 화면 추적 (중복 오픈 방지 / 닫기) */
	UPROPERTY(Transient)
	TMap<FGameplayTag, TWeakObjectPtr<UCommonActivatableWidget>> OpenScreens;
};
