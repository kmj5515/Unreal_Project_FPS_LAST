#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LastFPSUIThemeReceiver.generated.h"

class ULastFPSUIThemeAsset;

UINTERFACE(MinimalAPI, NotBlueprintable)
class ULastFPSUIThemeReceiver : public UInterface
{
	GENERATED_BODY()
};

/**
 * 테마를 받아 자신을 칠할 수 있는 위젯.
 *
 * 공통 부모 클래스를 두면 이미 다른 기반 클래스를 쓰는 위젯(CommonUI 계열 등)이 참여할 수 없고
 * 상속 계층이 표시 책임까지 떠안는다. 인터페이스로 두어 원하는 위젯만 골라 참여하게 한다.
 */
class LASTFPS_API ILastFPSUIThemeReceiver
{
	GENERATED_BODY()

public:
	/** 테마 값을 자신의 표시 상태에 옮긴다. 자식 위젯 전파는 호출부(LastFPSUITheme)가 맡는다. */
	virtual void ApplyUITheme(const ULastFPSUIThemeAsset& Theme) = 0;
};
