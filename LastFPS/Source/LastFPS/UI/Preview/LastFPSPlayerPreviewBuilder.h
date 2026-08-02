#pragma once

#include "CoreMinimal.h"

struct FLastFPSPreviewSubject;

/**
 * 로컬 플레이어의 "지금 모습"을 프리뷰 무대에 올릴 대상으로 옮긴다.
 *
 * 무대는 무엇이 올라오는지 알지 않고, 화면은 무엇을 어떻게 조립하는지 알 필요가 없다.
 * 그 사이를 잇는 변환만 여기 한 곳에 둔다. HUB 껍데기와 장비 화면이 같은 모습을 보여야 하므로
 * 조립 규칙이 두 곳으로 갈리면 곧바로 어긋난다.
 */
namespace LastFPSPlayerPreview
{
	/**
	 * 플레이어 폰의 메시와 현재 장착 무기로 무대 대상을 채운다.
	 *
	 * @param WorldContext 로컬 플레이어를 찾을 기준 객체.
	 * @param OutSubject   성공했을 때만 채워진다. 실패 시 값은 건드리지 않는다.
	 * @return 세울 메시를 찾았으면 true. 폰이 아직 없거나 메시가 비어 있으면 false.
	 */
	LASTFPS_API bool BuildSubject(const UObject* WorldContext, FLastFPSPreviewSubject& OutSubject);
}
