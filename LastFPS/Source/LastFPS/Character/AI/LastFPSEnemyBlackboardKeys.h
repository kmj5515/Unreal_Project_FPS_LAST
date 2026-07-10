#pragma once

#include "CoreMinimal.h"

/**
 * 적 전투 BT 가 사용하는 Blackboard 키 이름 모음.
 * 컨트롤러·서비스·태스크가 같은 문자열을 공유해 오타로 인한 미연결을 막는다.
 * 에디터에서 Blackboard 에셋(BB_Enemy)을 만들 때 아래와 같은 타입/이름으로 키를 추가할 것:
 *   - TargetActor    : Object (Base Class = Actor)
 *   - bInAttackRange : Bool
 *   - TargetLocation : Vector   (원거리/이동 예측용, 선택)
 */
namespace LastFPSEnemyBBKeys
{
	/** 현재 교전 대상(플레이어 Pawn). 없으면 미설정 → 유휴 상태로 분기. */
	inline const FName TargetActor(TEXT("TargetActor"));

	/** 타깃이 AttackRange 안에 있는지. 추격 vs 공격 분기용. */
	inline const FName bInAttackRange(TEXT("bInAttackRange"));

	/** 타깃의 마지막 알려진 위치. MoveTo 예비/원거리 조준에 사용(선택). */
	inline const FName TargetLocation(TEXT("TargetLocation"));
}
