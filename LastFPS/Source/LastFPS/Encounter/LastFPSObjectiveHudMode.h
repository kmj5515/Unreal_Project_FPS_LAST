#pragma once

#include "CoreMinimal.h"
#include "LastFPSObjectiveHudMode.generated.h"

/**
 * 화면 하나를 점유하는 목표 표시 방식이다.
 *
 * 점령·방어·보스는 각각 표시 형태가 다르고 동시에 띄우지 않는다는 규칙이 있어,
 * 상호 배타 슬롯 하나를 두고 이 값으로 어느 표시를 켤지 고른다.
 * 값을 해석하는 곳은 HUD 프레젠터 한 곳뿐이며, 목표 컴포넌트는 데이터를 전달만 한다.
 */
UENUM(BlueprintType)
enum class ELastFPSObjectiveHudMode : uint8
{
	/** 표시 없음 — 목표가 없거나 끝난 상태다. */
	None	UMETA(DisplayName="없음"),

	/** 점령 — 게이지가 차오르는 형태. */
	Capture	UMETA(DisplayName="점령 게이지"),

	/** 방어 — 남은 시간을 표기하는 형태. */
	Defend	UMETA(DisplayName="방어 시간"),

	/** 보스 — 체력바 형태. */
	Boss	UMETA(DisplayName="보스 체력")
};
