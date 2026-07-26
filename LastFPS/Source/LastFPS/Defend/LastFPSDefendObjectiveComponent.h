#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "LastFPSDefendObjectiveComponent.generated.h"

class ALastFPSDefendableDeviceActor;
class ULastFPSQuestSubsystem;

/** 방어 진행률(0~1) 변경 — HUD 게이지용. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSDefenseProgress, float, Progress01);

/** 버티기 시간 완주 = 방어 성공. ZoneTag 로 퀘스트 DefendZone 목표와 매칭. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSDefenseSucceeded, FGameplayTag, ZoneTag);

/** 대상 파괴 = 방어 실패. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSDefenseFailed, FGameplayTag, ZoneTag);

/**
 * 수호(방어) 목표의 승패 규칙을 서버 권한으로 소유하는 컴포넌트.
 * 적 스폰(웨이브)은 ALastFPSRoomEncounterRuntime, 지킬 대상 상태는 ALastFPSDefendableDeviceActor 가
 * 각각 소유하고, 이 컴포넌트는 "정해진 시간 버티면 성공 / 대상 파괴되면 실패"만 판정한다(책임 분리).
 * 성공·실패는 델리게이트로만 알린다 — 퀘스트 진행(NotifyObjectiveDefended)과 실패 리스타트 연결은
 * ZoneTag 를 받은 브릿지(BP/에디터)가 담당한다. 실패 시 최소 리셋은 대상 내구도 복구 후 재시작.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSDefendObjectiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSDefendObjectiveComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 방어 시작 (서버 권한). 대상 파괴 구독 + 버티기 타이머 시작. 이미 진행 중이면 무시. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Defend")
	void StartDefense();

	/** 진행 중인 방어를 중단 (성공/실패 판정 없이). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Defend")
	void StopDefense();

	/** 0~1 진행률 (경과 / 버티기 시간). */
	UFUNCTION(BlueprintPure, Category="LastFPS|Defend")
	float GetProgress01() const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDefenseProgress OnDefenseProgress;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDefenseSucceeded OnDefenseSucceeded;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDefenseFailed OnDefenseFailed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 지킬 대상 장치 (레벨에서 지정). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="LastFPS|Defend")
	TObjectPtr<ALastFPSDefendableDeviceActor> Device;

	/** 성공까지 버텨야 하는 시간(초) — 밸런스 값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend", meta=(ClampMin="1.0", Units="s"))
	float HoldDuration = 60.f;

	/** 이 구역 태그로 퀘스트 DefendZone 목표와 매칭 (성공/실패 델리게이트로 전달). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend")
	FGameplayTag ZoneTag;

	/** 진행 갱신/타이머 주기(초). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend", meta=(ClampMin="0.05", Units="s"))
	float UpdateInterval = 0.25f;

	/** 실패 시 대상 내구도를 복구하고 자동으로 다시 시작할지(최소 리셋). false 면 실패만 알리고 멈춘다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend")
	bool bRestartOnFail = true;

	/** BeginPlay 에서 자동 시작할지. false 면 StartDefense 를 외부(트리거 등)에서 호출. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend")
	bool bAutoStart = true;

private:
	UFUNCTION()
	void HandleDeviceDestroyed();

	UFUNCTION()
	void OnRep_Elapsed();

	/** 타이머 콜백(서버) — 경과 누적, 완주 시 성공 승격. */
	void TickHold();
	void Succeed();
	void Fail();
	void BroadcastProgress();

	bool HasAuthority() const;

	/** 로컬 GameInstance 의 퀘스트 서브시스템 (없으면 null). */
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	UPROPERTY(ReplicatedUsing=OnRep_Elapsed)
	float ElapsedSeconds = 0.f;

	FTimerHandle HoldTimerHandle;
	bool bActive = false;
	bool bBoundToDevice = false;
};
