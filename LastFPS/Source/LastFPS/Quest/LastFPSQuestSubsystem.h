#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/TimerHandle.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "Quest/LastFPSObjectiveTracker.h"
#include "LastFPSQuestSubsystem.generated.h"

class UDataTable;
class USceneComponent;
class ULastFPSEconomySubsystem;
class ULastFPSRoomEncounterSubsystem;

/** 퀘스트 상태/진행이 바뀔 때 브로드캐스트 — 임무 화면/트래커 UI 갱신용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSQuestStateChanged);

/** 무전 자막/사운드 출력 요청 브로드캐스트 — HUD 무전 위젯 바인딩용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLastFPSRadioTransmission, const FLastFPSRadioTransmissionData&, RadioData);

/**
 * 화면 마커 대상 1건 — 진행중 퀘스트의 위치 목표(ReachLocation)를 월드 좌표로 노출.
 * 거리(m)는 플레이어 위치에 따라 매 프레임 바뀌므로 여기 담지 않고 HUD 위젯이 계산한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSObjectiveWaypoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FName QuestId;

	/** 바닥 투영 캐시를 구분하는 위치 목표 식별자이다. */
	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FGameplayTag LocationTag;

	/**
	 * 목적지가 아니라 이동 동선을 안내하는 중간 지점인가.
	 * 레벨에 바닥으로 배치된 좌표라 HUD 가 별도 바닥 투영을 하지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Quest")
	bool bIsRoutePoint = false;
};

/**
 * 퀘스트 1건의 런타임 상태 — DataTable 정적 정의와 분리해 서브시스템이 소유한다.
 * Progress/Baseline 인덱스는 해당 행 Objectives 배열 순서와 1:1.
 */
struct FLastFPSQuestRuntimeState
{
	ELastFPSQuestStatus Status = ELastFPSQuestStatus::NotStarted;
	TArray<int32> Progress;   // 목표별 현재 진행 (0..RequiredCount)
	TArray<int32> Baseline;   // 목표별 수락 시점 기준선 (AcquireItem: 수락 때 GetItemCount)
};

USTRUCT(BlueprintType)
struct FLastFPSDungeonQuestMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FString MapKeyword;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName QuestId;
};

/**
 * 아웃게임 퀘스트 — GameInstanceSubsystem (Economy/Loadout 과 동일 패턴).
 *
 * 정적 정의(DT_QuestData)와 런타임 상태(진행/완료/수령)를 분리한다. 진행 판정은 "수락 시점
 * 보유량을 기준선으로 캡처 → 이후 증가분" 방식이라 시드 재고가 소급 완료되지 않는다.
 * 완료/수령은 단조 전이(NotStarted→InProgress→Completed→Claimed)로만 바뀌며, 보상은
 * Completed→Claimed 전이에서 EconomySubsystem 으로 딱 1회 지급한다(중복지급 래치).
 *
 * 범위 메모: 아웃게임 로컬(무복제) 가정 — Economy 와 동일. SaveGame 영속화는 추후.
 */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 런타임 상태 (없으면 NotStarted) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	ELastFPSQuestStatus GetStatus(FName QuestId) const;

	/** 목표별 현재 진행 (인덱스 = 행 Objectives 순서). 범위 밖이면 0. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	int32 GetObjectiveProgress(FName QuestId, int32 ObjectiveIndex) const;

	/** 목표별 실제 요구량. ClearEncounter는 EncounterTable의 적 수 합계를 반환한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	int32 GetObjectiveRequiredCount(FName QuestId, int32 ObjectiveIndex) const;

	/** 목표 달성(완료 또는 수령완료) 여부 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	bool IsComplete(FName QuestId) const;

	/** 보상 수령 가능(Completed) 여부 — UI 버튼 활성 판단용 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	bool IsClaimable(FName QuestId) const;

	/**
	 * 퀘스트 수락 — NotStarted→InProgress, 목표별 기준선(현재 보유량) 캡처.
	 * 이미 진행/완료/수령 상태면 아무것도 안 하고 false.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	bool AcceptQuest(FName QuestId);

	/**
	 * 보상 수령 — Completed 일 때만 Claimed 로 1회 전이하며 Economy 에 크레딧/아이템 지급.
	 * 완료 전이거나 이미 수령했으면 false.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	bool TryClaimReward(FName QuestId);

	/** 정적 정의 접근 (UI 목록 생성용 — 테이블 단일 소스) */
	const UDataTable* GetQuestTable() const;
	const FLastFPSQuestData* FindQuest(FName QuestId) const;

	// ── 외부 이벤트 통지 (목표 진행 push) ────────────────────────────

	/** 처치 통지 — 서버 사망 브릿지가 오너 클라에서 호출. KillTarget 목표를 누적. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyObjectiveKill(FGameplayTag EnemyTag);

	/** 대화 통지 — NPC 상호작용 시작 시 호출. TalkToNPC 목표를 누적. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyTalkedToNPC(FName NPCRowName);

	/** 인카운터 클리어 통지 — RoomEncounterSubsystem 이벤트에서 호출. ClearEncounter 목표를 처리. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyEncounterCleared(FName EncounterId);

	/** 방 인카운터의 실제 처치 수를 ClearEncounter 목표에 절대 진행값으로 반영한다. */
	UFUNCTION()
	void NotifyEncounterProgress(FName EncounterId, int32 DefeatedEnemyCount, int32 TotalEnemyCount);

	/** 단일 무전 대사를 HUD 자막 위젯으로 전송한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest|Radio")
	void TriggerRadioTransmission(const FLastFPSRadioTransmissionData& RadioData);

	/** 연속 무전 대사 목록을 HUD 자막 위젯으로 전송한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest|Radio")
	void TriggerRadioTransmissions(const TArray<FLastFPSRadioTransmissionData>& RadioDataArray);

	/** FName 배열로 지정된 무전 대사를 RadioTable 에서 조회하여 HUD 자막 위젯으로 전송한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest|Radio")
	void TriggerRadioByIds(const TArray<FName>& RadioIds);

	// ── 위치 마커 등록소 (ReachLocation 위치 소스 / HUD 공용) ─────────

	/** 위치 마커 등록 (ULastFPSObjectiveMarkerComponent 가 BeginPlay 에서 호출). */
	void RegisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker);
	/** 위치 마커 해제 (EndPlay). 같은 컴포넌트일 때만 제거. */
	void UnregisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker);
	/** 태그에 등록된 월드 위치 조회. 없거나 파괴됐으면 false. */
	bool GetTrackedLocation(FGameplayTag LocationTag, FVector& OutLocation) const;

	/**
	 * 위치 목표의 도달 지점 — 등록된 마커 컴포넌트를 우선 사용하고, 없으면
	 * 레벨에 배치된 경로(TargetId)의 마지막 지점을 쓴다. 둘 다 없으면 false.
	 */
	bool ResolveObjectiveLocation(const FLastFPSQuestObjective& Objective, FVector& OutLocation) const;

	/** 볼륨 트리거 도달 통지 (ULastFPSObjectiveTriggerComponent 오버랩 변화 시). 즉시 재계산+브로드캐스트. */
	void NotifyLocationTriggerChanged(FGameplayTag LocationTag, bool bPlayerInside);

	/** 해당 위치 태그의 볼륨 트리거에 로컬 플레이어가 현재 들어와 있는가 (ReachLocation 판정 보조). */
	bool IsLocationTriggerActive(FGameplayTag LocationTag) const;

	/**
	 * 진행중 퀘스트의 미완료 위치 목표(ReachLocation) 중 위치를 확정할 수 있는 것들. HUD 마커용.
	 * 목적지 앞에 배치 경로의 중간 지점(bIsRoutePoint)이 진행 순서대로 먼저 담긴다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void GetActiveWaypoints(TArray<FLastFPSObjectiveWaypoint>& OutWaypoints) const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest")
	FOnLastFPSQuestStateChanged OnQuestStateChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest|Radio")
	FOnLastFPSRadioTransmission OnRadioTransmission;

protected:
	/** 퀘스트 정의 테이블 (DT_QuestData) — DefaultGame.ini 로 지정 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest")
	TSoftObjectPtr<UDataTable> QuestTable;

	/** ClearEncounter 목표 요구량과 던전 트리거 매칭에 사용하는 방 인카운터 정의 테이블. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Dungeon", meta=(RowType="/Script/LastFPS.LastFPSRoomEncounterData"))
	TSoftObjectPtr<UDataTable> EncounterTable;

	/** 무전 대사 테이블 (DT_RadioTransmission) — DefaultGame.ini 로 지정 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Radio")
	TSoftObjectPtr<UDataTable> RadioTable;

	/** 던전 맵 키워드와 자동 진행할 퀘스트 ID 매핑 (DefaultGame.ini 로 확장 가능) */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Dungeon")
	TArray<FLastFPSDungeonQuestMapping> DungeonMapQuestMap;

	/** 기본 던전 퀘스트 ID (매핑 테이블 미설정 시 폴백) */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Dungeon")
	FName DefaultDungeonQuestId;

	/** ReachLocation 도달 판정 폴 주기(초). 위치 목표가 활성일 때만 타이머가 돈다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest", meta=(ClampMin=0.05))
	float LocationPollInterval = 0.2f;

	/**
	 * 경로 식별 태그로 인정할 상위 태그. 이 하위의 등록된 Gameplay Tag 가 붙은 배치 액터만
	 * 이동 동선 지점으로 본다. 비우면 등록된 모든 태그를 허용한다.
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Path")
	FGameplayTag PathRouteTagRoot;

	/** 동선 진행 순서를 담은 태그의 접두사 (예: "Order." → Order.03). */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Path")
	FString PathOrderTagPrefix = TEXT("Order.");

	/** 이 거리(cm) 안으로 들어오면 해당 동선 지점에 도달한 것으로 보고 다음 지점을 안내한다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest|Path", meta=(ClampMin="0"))
	float ReachedPointDistance = 500.f;
private:
	ULastFPSEconomySubsystem* GetEconomy() const;
	const UDataTable* GetEncounterTable() const;
	int32 ResolveObjectiveRequiredCount(const FLastFPSQuestObjective& Objective) const;

	// ── 이동 동선 (레벨에 배치된 지점 액터가 단일 소스) ─────────────

	/** 월드의 경로 지점 액터를 스캔해 목표 태그별 동선을 구성한다(월드 초기화 시 1회). */
	void ScanObjectivePaths(UWorld& World);

	/** 액터 태그에서 (경로 식별 태그, 순서)를 해석한다. 경로 지점이 아니면 false. */
	bool ParsePathPointTags(const AActor& Actor, FGameplayTag& OutRouteTag, int32& OutOrder) const;

	/** 안내 지점을 From 기준으로 전진시킨다 — 닿으면 다음 지점, 뒤로는 돌아가지 않는다. */
	void AdvanceRouteProgress(FGameplayTag RouteTag, const FVector& From);

	/** 지금 안내 중인 지점 1개. 마지막 지점이면 bOutIsDestination 이 true. */
	bool GetCurrentRoutePoint(FGameplayTag RouteTag, FVector& OutLocation, bool& bOutIsDestination) const;

	/** 동선의 마지막 지점(= 도달 지점). */
	bool GetRouteDestination(FGameplayTag RouteTag, FVector& OutLocation) const;

	FDelegateHandle OnWorldInitHandle;
	void HandlePostWorldInitialization(const FActorsInitializedParams& Params);
	void BindEncounterEvents(UWorld& World);
	void UnbindEncounterEvents();

	/** 맵 키워드에 매핑된 던전 퀘스트를 1회 자동 수락한다. */
	void AcceptDungeonQuestForMap(UWorld& World);

	/** 던전 퀘스트를 이미 자동 수락한 월드 (동일 월드 중복 처리 방지, 재입장 시 재수행 허용). */
	TWeakObjectPtr<UWorld> DungeonQuestAcceptedWorld;
	const UDataTable* GetRadioTable() const;
	const FLastFPSRadioTransmissionData* FindRadioTransmission(FName RadioId) const;

	/** 유형별 목표 트래커 구성 (Initialize 1회). */
	void BuildTrackers();
	const ILastFPSObjectiveTracker* GetTracker(ELastFPSObjectiveType Type) const;

	/** pull형 트래커가 참조할 현재 외부 상태(Economy/로컬 폰 위치) 구성. */
	FLastFPSObjectiveEvalContext MakeEvalContext() const;

	/** 부팅 시 각 행의 Status(초기 시드)로 런타임 상태를 만든다. InProgress 시드는 기준선도 캡처. */
	void SeedRuntimeStates();

	/**
	 * 시작 시 참조 무결성 검사 — 목표 대상/보상 아이템 RowId 가 DT_ItemData 에 있는지 에러 로그.
	 * (없으면: 목표는 영원히 진행 불가, 보상은 지급 시 무시됨.) ItemTable 미설정 시엔 건너뜀.
	 */
	void ValidateReferences() const;

	/** 보유 변동 시 — 진행중 퀘스트 진행 재계산 + 완료 전이. */
	UFUNCTION()
	void HandleInventoryChanged();

	/** 위치 폴 타이머 콜백 — 경로 안내 지점 갱신 후 pull형(위치 등) 재계산. */
	void HandleLocationPoll();

	/** 진행중 위치 목표의 경로 안내 지점을 플레이어 위치로 전진시킨다. */
	void UpdateRouteProgress(const FLastFPSObjectiveEvalContext& Context);

	/** 모든 진행중 퀘스트의 pull형 목표 재계산 + 완료 승격. 변경 시 true(브로드캐스트는 호출부). */
	bool RecomputeAllActive();

	/** 한 퀘스트의 pull형 진행을 절대상태에서 재계산. 완료 도달 시 Completed 로 단조 승격. 변경 시 true. */
	bool RecomputeProgress(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def);

	/** 외부 이벤트를 진행중 퀘스트들에 적용(push형 목표 누적). 변경 시 true(브로드캐스트/연쇄는 호출부). */
	bool ApplyObjectiveEventToActive(const FLastFPSObjectiveEvent& Event);
	/** 한 퀘스트에 이벤트 적용. 변경 시 true. */
	bool ApplyEventToQuest(FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def, const FLastFPSObjectiveEvent& Event);

	/** 모든 목표 충족 시 Completed 로 단조 승격. 승격했으면 true. */
	bool CheckCompletion(FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def) const;

	/**
	 * 목표가 이번 변경으로 처음 충족됐다면 그 목표의 완료 무전을 재생한다.
	 * (도착 무전을 트리거 액터가 아니라 목표 데이터가 소유하도록 일원화한 지점)
	 */
	void NotifyObjectiveCompleted(
		const FLastFPSQuestObjective& Objective,
		int32 PreviousProgress,
		int32 NewProgress,
		int32 RequiredCount);

	/** 수락 코어(가드 없음) — InProgress 전이 + 기준선 + 즉시 재계산. */
	bool AcceptQuestInternal(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def);

	/** 보상 지급(크레딧/아이템) + 완료 토스트. 상태 래치/전이는 호출부 책임. */
	void GrantReward(const FLastFPSQuestData& Def);

	/**
	 * 완료→(자동)수령→다음 퀘스트 해금/수락 연쇄를 안정될 때까지 처리. 변경 시 true.
	 * 보상 지급이 유발하는 인벤토리 브로드캐스트 재진입은 가드로 차단(외부 루프가 이어서 처리).
	 */
	bool ProcessQuestTransitions();

	/** 다음 퀘스트 해금 — QuestGiverNPC 없으면 즉시 수락, 있으면 NotStarted 로 해금만. 변경 시 true. */
	bool AdvanceToNext(FName NextQuestId);

	/** 목표별 기준선 캡처 (트래커 위임). */
	void CaptureBaseline(const FLastFPSQuestData& Def, FLastFPSQuestRuntimeState& State) const;

	/** 상태 변경 통지 — 위치 폴 타이머 갱신 후 델리게이트 브로드캐스트. */
	void BroadcastStateChanged();

	/** 진행중 ReachLocation 목표 유무에 따라 위치 폴 타이머를 켜고/끈다. */
	void UpdateLocationPollTimer();

	/** 완료 토스트 (로컬 PC 의 ShowNotice). */
	void NotifyRewardGranted(const FLastFPSQuestData& Def) const;

	/** 수령 알림 본문 — 제목 + 지급된 보상(크레딧/아이템) 내역. 구조화 보상이 비면 RewardText 폴백. */
	FText BuildRewardMessage(const FLastFPSQuestData& Def) const;

	TMap<FName, FLastFPSQuestRuntimeState> RuntimeStates;

	/** 목표 유형 → 판정 트래커 (Initialize 에서 구성). */
	TMap<ELastFPSObjectiveType, TUniquePtr<ILastFPSObjectiveTracker>> Trackers;

	/** 위치 태그 → 마커 컴포넌트 (레벨 액터 수명이라 약참조). */
	TMap<FGameplayTag, TWeakObjectPtr<USceneComponent>> LocationMarkers;

	/** 위치 태그 → 현재 겹친 볼륨 트리거 수 (0 초과면 도달 판정 충족). */
	TMap<FGameplayTag, int32> LocationTriggerOverlaps;

	/** 위치 태그 → 진행 순서대로 정렬된 동선 지점 (월드 스캔 결과, 배치 액터는 정적이라 값 캐시). */
	TMap<FGameplayTag, TArray<FVector>> ObjectiveRoutes;

	/** 위치 태그 → 현재 안내 중인 동선 지점 인덱스 (런타임 상태, 배치 데이터와 분리). */
	TMap<FGameplayTag, int32> RouteCursors;

	/** 전이 처리 재진입 가드 (보상 지급→인벤토리 브로드캐스트 재귀 차단). */
	bool bProcessingTransitions = false;

	/** 부팅 시드 중에는 이미 충족된 목표의 완료 무전을 재생하지 않는다. */
	bool bSuppressObjectiveRadio = false;

	FTimerHandle LocationPollTimerHandle;
	bool bInventorySubscribed = false;
	TWeakObjectPtr<ULastFPSRoomEncounterSubsystem> BoundEncounterSubsystem;
};
