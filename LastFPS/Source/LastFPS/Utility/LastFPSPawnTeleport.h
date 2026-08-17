#pragma once

#include "CoreMinimal.h"

class APawn;

/**
 * 서버 권한에서 폰을 다른 지점으로 옮기는 공용 경로다.
 *
 * 스트리밍 전환(목적지 이동)과 룸 인카운터(뒤처진 파티원 모으기)가 같은 요구를 갖는다 —
 * 파티원이 한 점에 몰리지 않게 빈 자리를 찾고, 이동 후 관성과 컨트롤 회전을 정리해
 * 시뮬레이티드 프록시가 이전 위치에서 미끄러지지 않게 해야 한다.
 * 두 곳에 같은 절차를 따로 두면 한쪽만 고쳐지므로 여기로 모은다.
 */
namespace LastFPSPawnTeleport
{
	/**
	 * 폰을 목적지로 옮긴다. 반드시 서버에서 호출한다.
	 * 이동에 실패하면(빈 자리를 못 찾는 등) false 를 돌려주고 폰은 원래 자리에 남는다.
	 */
	LASTFPS_API bool TeleportPawnTo(APawn& Pawn, const FTransform& Destination);
}
