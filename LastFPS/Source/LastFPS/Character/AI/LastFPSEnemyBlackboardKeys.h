#pragma once

#include "CoreMinimal.h"

/**
 * 적 전투 BT 가 사용하는 Blackboard 키 이름 모음.
 * 컨트롤러·서비스·태스크가 같은 문자열을 공유해 오타로 인한 미연결을 막는다.
 * 에디터에서 Blackboard 에셋(BB_Enemy)을 만들 때 아래와 같은 타입/이름으로 키를 추가할 것:
 *   - TargetActor      : Object (Base Class = Actor)
 *   - TargetDistance   : Float
 *   - bHasLineOfSight  : Bool
 *   - bTargetTooClose  : Bool
 *   - TargetLocation   : Vector   (원거리/이동 예측용, 선택)
 *   - KiteLocation     : Vector   (EQS 카이팅 목적지)
 */
namespace LastFPSEnemyBBKeys
{
	/** 현재 교전 대상(플레이어 Pawn). 없으면 미설정 → 유휴 상태로 분기. */
	inline const FName TargetActor(TEXT("TargetActor"));

	/** 현재 Pawn과 타깃 사이의 월드 거리다. 공격별 거리 Decorator가 사용한다. */
	inline const FName TargetDistance(TEXT("TargetDistance"));

	/** 타깃이 시야(직선 트레이스)에 보이는지. 원거리 공격을 벽 너머로 쏘지 않게 게이팅. */
	inline const FName bHasLineOfSight(TEXT("bHasLineOfSight"));

	/** 타깃이 KeepDistance 보다 가까운지. 참이면 카이팅(뒤로 빠지기) 분기. */
	inline const FName bTargetTooClose(TEXT("bTargetTooClose"));

	/** 타깃의 마지막 알려진 위치. MoveTo 예비/원거리 조준에 사용(선택). */
	inline const FName TargetLocation(TEXT("TargetLocation"));

	/** EQS 가 고른 카이팅 목적지. RunEQS → 이 키에 기록 → MoveTo 가 사용. */
	inline const FName KiteLocation(TEXT("KiteLocation"));
}
