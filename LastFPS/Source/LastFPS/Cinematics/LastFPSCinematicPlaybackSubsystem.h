#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "Cinematics/LastFPSCinematicTypes.h"
#include "LastFPSCinematicPlaybackSubsystem.generated.h"

class ALevelSequenceActor;
class ULevelSequencePlayer;
struct FStreamableHandle;

/** 컷신 재생이 시작됐다. 화면 측이 스킵 안내를 띄우는 데 쓴다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLastFPSCinematicStarted,
	bool, bSkippable);

/** 컷신 재생이 끝났다. bSkipped 면 플레이어가 건너뛴 것이다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLastFPSCinematicFinished,
	bool, bSkipped);

/**
 * 컷신 재생 슬롯 하나를 소유하는 재생기다.
 *
 * 컷신은 화면 전체를 점유하므로 동시 재생을 허용하지 않고 슬롯을 하나만 둔다.
 * 요청자는 FLastFPSCinematicPlayback 데이터만 넘기며, 이 서브시스템은 요청 주체가
 * 퀘스트인지 인카운터인지 알지 않는다 — 새 연출 지점이 생겨도 여기 배선은 그대로다.
 *
 * 클라이언트 로컬 표시 전용이라 복제하지 않는다. 각 머신이 자기 화면을 소유한다.
 */
UCLASS()
class LASTFPS_API ULastFPSCinematicPlaybackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 월드 컨텍스트에서 서브시스템을 얻는 공용 접근자. 전용 서버/무월드 컨텍스트면 nullptr. */
	static ULastFPSCinematicPlaybackSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * 컷신 재생을 요청한다. 시퀀스가 비었거나 맵이 맞지 않거나 이미 재생 중이면 false.
	 * 시퀀스는 소프트 참조라 비동기 로드 후 재생하며, 요청 수락 시점에 true 를 반환한다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Cinematic")
	bool RequestPlayback(const FLastFPSCinematicPlayback& Request);

	/** 재생 중인 컷신을 플레이어 의사로 건너뛴다. 스킵 불가 설정이면 무시하고 false. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Cinematic")
	bool TrySkip();

	/** 재생 중인 컷신을 즉시 종료한다(맵 전환·강제 중단용). 스킵 가능 여부를 따지지 않는다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Cinematic")
	void StopPlayback();

	UFUNCTION(BlueprintPure, Category="LastFPS|Cinematic")
	bool IsPlaying() const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Cinematic")
	FOnLastFPSCinematicStarted OnCinematicStarted;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Cinematic")
	FOnLastFPSCinematicFinished OnCinematicFinished;

	virtual void Deinitialize() override;

private:
	/** 요청의 맵 조건이 현재 월드와 맞는가. 조건이 비어 있으면 항상 참. */
	bool MatchesRequiredWorld(const FLastFPSCinematicPlayback& Request) const;

	/** 지연 시간이 있으면 타이머로 미루고, 없으면 즉시 로드를 시작한다. */
	void ScheduleLoad();

	void BeginLoad();

	/** 로드 완료 콜백 — 로드된 시퀀스로 플레이어를 만들고 재생을 시작한다. */
	void HandleSequenceLoaded();

	/** 재생 종료 경로의 단일 출구. 액터 정리 + 슬롯 반납 + 종료 통지를 한 곳에서 한다. */
	void FinishPlayback(bool bSkipped);

	UFUNCTION()
	void HandlePlayerFinished();

	/** 지금 슬롯이 들고 있는 요청. Sequence 가 비면 유휴 상태다. */
	UPROPERTY(Transient)
	FLastFPSCinematicPlayback PendingRequest;

	/** 재생 중인 시퀀스 액터. 종료 시 파괴한다. */
	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

	TSharedPtr<FStreamableHandle> SequenceLoadHandle;

	FTimerHandle StartDelayTimerHandle;

	/** 요청 수락(로드 대기 포함)부터 종료까지 참. 슬롯 점유 판정의 단일 소스다. */
	bool bSlotOccupied = false;
};
