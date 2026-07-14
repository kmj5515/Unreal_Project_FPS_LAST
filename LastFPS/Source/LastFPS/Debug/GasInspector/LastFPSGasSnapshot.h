#pragma once

#include "CoreMinimal.h"

/**
 * GAS 런타임 인스펙터의 데이터 계약.
 *
 * ASC(구체 GAS)와 표시(Slate 뷰)를 분리하기 위한 순수 스냅샷이다.
 * 빌더가 ASC를 읽어 이 구조체를 채우고, 패널은 이 구조체만 소비한다.
 * UObject를 참조하지 않으므로 수명/스레드 걱정 없이 값으로 넘길 수 있다.
 */

// 단일 어트리뷰트의 현재값과 베이스값.
struct FLastFPSGasAttributeSnapshot
{
	FString Name;
	float BaseValue = 0.f;
	float CurrentValue = 0.f;
};

// 적용 중인 게임플레이 이펙트 1건. Duration이 0 이하이면 무한/즉시로 간주한다.
struct FLastFPSGasEffectSnapshot
{
	FString Name;
	float Duration = 0.f;
	float TimeRemaining = 0.f;
	int32 StackCount = 1;
};

// 부여된 어빌리티 1건.
struct FLastFPSGasAbilitySnapshot
{
	FString Name;
	bool bActive = false;
};

// 한 대상(플레이어 또는 스포이드로 찍은 캐릭터)의 GAS 상태 스냅샷.
struct FLastFPSGasSnapshot
{
	bool bValid = false;
	FString DisplayName;
	FString OwnerClassName;
	TArray<FLastFPSGasAttributeSnapshot> Attributes;
	TArray<FLastFPSGasEffectSnapshot> Effects;
	TArray<FString> OwnedTags;
	TArray<FLastFPSGasAbilitySnapshot> Abilities;

	void Reset()
	{
		bValid = false;
		DisplayName.Reset();
		OwnerClassName.Reset();
		Attributes.Reset();
		Effects.Reset();
		OwnedTags.Reset();
		Abilities.Reset();
	}
};
