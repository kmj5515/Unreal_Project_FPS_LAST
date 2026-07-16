#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "LastFPSEnemyMeleeAttackData.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 적 근접 공격의 표현과 판정 설정이다.
 * 어빌리티는 실행 절차만 담당하고 적별 밸런스와 애니메이션 차이는 이 데이터로 확장한다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSEnemyMeleeAttackData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULastFPSEnemyMeleeAttackData();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Animation", meta=(ClampMin="0.01"))
	float MontagePlayRate = 1.f;

	/** 이 태그의 Send Gameplay Event 노티파이가 발생한 프레임에 판정을 실행한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Animation")
	FGameplayTag HitEventTag;

	/** 캐릭터 로컬 좌표 기준 판정 시작 위치다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Trace")
	FVector TraceStartOffset = FVector(40.f, 0.f, 60.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Trace", meta=(ClampMin="0.0", Units="cm"))
	float TraceDistance = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Trace", meta=(ClampMin="1.0", Units="cm"))
	float TraceRadius = 55.f;

	/** 꺼져 있으면 Sweep에서 가장 먼저 확인된 유효 대상 하나만 공격한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Trace")
	bool bHitMultipleTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Damage")
	FLastFPSDamageRange DamageRange;

	/** 데미지 외 상태 효과도 함께 구성할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Damage")
	TArray<TSubclassOf<UGameplayEffect>> EffectsOnHit;
};
