#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "LastFPSCaptureZoneComponent.generated.h"

class ULastFPSQuestSubsystem;

/** 점령 진행률(0~1) 변경 — HUD 게이지용. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSCaptureProgress, float, Progress01);

/** 점령 완료 — ZoneTag 로 퀘스트 CaptureZone 목표와 매칭. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSCaptureComplete, FGameplayTag, ZoneTag);

/**
 * 점령 목표의 볼륨 소스. 로컬 플레이어가 이 볼륨 안에 CaptureDuration 초 머무르면 점령 완료.
 * 방어(ULastFPSDefendObjectiveComponent)와 달리 "안에 있는 동안만" 진행이 차오르고, 나가면 멈춘다
 * (감소 없음 — 슬라이스 최소 규칙). 적 경합(안에 적 있으면 정지)은 추후 확장 지점.
 * 완료는 델리게이트로만 알리며, 퀘스트 진행(NotifyObjectiveCaptured) 연결은 ZoneTag 를 받은 브릿지가 담당.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSCaptureZoneComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	ULastFPSCaptureZoneComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 0~1 진행률 (경과 / 점령 시간). */
	UFUNCTION(BlueprintPure, Category="LastFPS|Capture")
	float GetProgress01() const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Capture")
	FOnLastFPSCaptureProgress OnCaptureProgress;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Capture")
	FOnLastFPSCaptureComplete OnCaptureComplete;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 점령까지 볼륨 안에 머물러야 하는 시간(초) — 밸런스 값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Capture", meta=(ClampMin="1.0", Units="s"))
	float CaptureDuration = 8.f;

	/** 이 구역 태그로 퀘스트 CaptureZone 목표와 매칭 (완료 델리게이트로 전달). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Capture")
	FGameplayTag ZoneTag;

	/** 진행 갱신/타이머 주기(초). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Capture", meta=(ClampMin="0.05", Units="s"))
	float UpdateInterval = 0.25f;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_Elapsed();

	/** 타이머 콜백(서버) — 볼륨 안일 때만 경과 누적, 완주 시 점령 완료. */
	void TickCapture();
	void SetPlayerInside(bool bInside);
	void BroadcastProgress();

	bool HasAuthority() const;

	/** 로컬 조종 플레이어 폰만 점령 주체로 인정 (퀘스트는 로컬 판정). */
	static bool IsLocalPlayerPawn(const AActor* Actor);

	/** 로컬 GameInstance 의 퀘스트 서브시스템 (없으면 null). */
	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	UPROPERTY(ReplicatedUsing=OnRep_Elapsed)
	float ElapsedSeconds = 0.f;

	FTimerHandle CaptureTimerHandle;
	bool bPlayerInside = false;
	bool bCaptured = false;
};
