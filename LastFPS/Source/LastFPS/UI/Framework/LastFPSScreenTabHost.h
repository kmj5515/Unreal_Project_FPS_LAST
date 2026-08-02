#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSScreenTabHost.generated.h"

class UCommonActivatableWidget;
struct FGameplayTag;

UINTERFACE(MinimalAPI, NotBlueprintable)
class ULastFPSScreenTabHost : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상단바를 유지한 채 콘텐츠만 교체하는 껍데기 화면(shell)이 구현한다.
 *
 * UIManagerSubsystem 이 로비 위젯을 직접 알면 UI 라우팅이 특정 화면 구현에 묶인다.
 * 어떤 태그를 자기 안에서 보여줄 수 있는지만 계약으로 노출해, 매니저는 "위임할 수 있는가"만 묻는다.
 */
class LASTFPS_API ILastFPSScreenTabHost
{
	GENERATED_BODY()

public:
	/** 이 화면 태그를 자기 안의 탭으로 들고 있는지. false 면 매니저가 기존처럼 레이어에 push 한다. */
	virtual bool HostsScreenTab(const FGameplayTag& ScreenTag) const = 0;

	/**
	 * 해당 탭을 전면에 세운다. 아직 만들어지지 않았다면 이때 생성한다.
	 * @return 표시된 위젯. 생성이나 전환에 실패하면 nullptr
	 */
	virtual UCommonActivatableWidget* ShowScreenTab(const FGameplayTag& ScreenTag) = 0;
};
