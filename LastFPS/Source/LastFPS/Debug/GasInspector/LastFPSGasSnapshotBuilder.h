#pragma once

#include "CoreMinimal.h"
#include "Debug/GasInspector/LastFPSGasSnapshot.h"

class UAbilitySystemComponent;
class AActor;

/**
 * ASC를 읽어 GAS 스냅샷을 만드는 순수 수집기.
 *
 * 상태를 갖지 않는 정적 유틸리티라 어떤 호출부에서도 재사용할 수 있다.
 * 특정 캐릭터/무기/게임모드를 열거하지 않고 ASC라는 안정적 계약만 소비한다.
 */
class FLastFPSGasSnapshotBuilder
{
public:
	// 액터에서 ASC를 해석(IAbilitySystemInterface 또는 전역 헬퍼)해 스냅샷을 만든다.
	// 유효한 ASC가 없으면 bValid=false 스냅샷을 반환한다.
	static FLastFPSGasSnapshot BuildFromActor(const AActor* Actor);

	// ASC를 직접 받아 스냅샷을 만든다. DisplayName/ClassName은 표시용 메타.
	static FLastFPSGasSnapshot BuildFromAbilitySystem(
	UAbilitySystemComponent* AbilitySystem,
		const FString& DisplayName,
		const FString& OwnerClassName);
};
