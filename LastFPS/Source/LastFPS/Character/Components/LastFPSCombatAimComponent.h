#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LastFPSCombatAimComponent.generated.h"

/** 전투 조준의 런타임 상태를 네트워크와 애니메이션에 전달하는 컴포넌트다. */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSCombatAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSCombatAimComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="LastFPS|Combat Aim")
	bool IsAimActive() const { return bAimActive; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Combat Aim")
	FVector GetAimTargetLocation() const { return FVector(AimTargetLocation); }

	UFUNCTION(BlueprintPure, Category="LastFPS|Combat Animation")
	bool IsFiringActive() const { return bFiringActive; }

	UFUNCTION(BlueprintPure, Category="LastFPS|Combat Animation")
	int32 GetShotSequence() const { return ShotSequence; }

	/** 서버에서 요청 소유자별 조준 상태를 갱신한다. */
	void SetAimTarget(UObject* RequestOwner, const FVector& TargetLocation);

	/** 현재 요청 소유자가 일치할 때만 조준 상태를 해제한다. */
	void ClearAimTarget(UObject* RequestOwner);

	/** 발사 구간의 시작을 원격 AnimBP에 전달한다. */
	void BeginFiring(UObject* RequestOwner);

	/** 실제 투사체 생성에 성공한 발사만 누적 번호로 전달한다. */
	void NotifyShotFired(UObject* RequestOwner);

	/** 현재 발사 요청 소유자가 일치할 때만 발사 구간을 종료한다. */
	void EndFiring(UObject* RequestOwner);

private:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Combat Aim", meta=(AllowPrivateAccess="true"))
	bool bAimActive = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Combat Aim", meta=(AllowPrivateAccess="true"))
	FVector_NetQuantize AimTargetLocation = FVector::ZeroVector;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Combat Animation", meta=(AllowPrivateAccess="true"))
	bool bFiringActive = false;

	/** 0으로 되돌리지 않아 AnimBP가 이전 프레임과 비교할 수 있는 누적 발사 번호다. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Combat Animation", meta=(AllowPrivateAccess="true"))
	int32 ShotSequence = 0;

	/** 서로 다른 Ability가 종료되며 최신 조준 요청을 지우는 것을 방지한다. */
	TWeakObjectPtr<UObject> ActiveRequestOwner;

	/** 조준 수명과 발사 수명이 다르므로 요청 소유권을 독립적으로 관리한다. */
	TWeakObjectPtr<UObject> ActiveFiringRequestOwner;
};
