#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/TimerHandle.h"
#include "Templates/Function.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "Data/Tables/LastFPSDialogueData.h"
#include "Quest/LastFPSObjectiveTracker.h"
#include "Quest/LastFPSQuestMarkerTypes.h"
#include "Quest/LastFPSNPCQuestOption.h"
#include "LastFPSQuestSubsystem.generated.h"

class UDataTable;
class USceneComponent;
class ULastFPSEconomySubsystem;
class ULastFPSRoomEncounterSubsystem;
struct FLastFPSPurchaseReceipt;

/** 퀘스트 상태/진행이 바뀔 때 브로드캐스트 — 임무 화면/트래커 UI 갱신용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSQuestStateChanged);

/** 무전 자막/사운드 출력 요청 브로드캐스트 — 한 요청에 포함된 대사 순서를 보존해 HUD에 전달한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLastFPSRadioTransmission,
	const TArray<FLastFPSRadioTransmissionData>&,
	RadioDataArray);

/** 무전 강제 종료 요청 브로드캐스트 — 시퀀스 종료/스킵 시 남아있는 자막을 지운다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSRadioTransmissionStop);

/**
 * 화면 마커 대상 1건 — 진행중 퀘스트의 안내 가능한 목표를 월드 좌표로 노출.
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

	/** 표시 측이 목표를 구분할 때 쓰는 위치 목표 식별자이다. */
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
 * 목표 1건의 안내 지점 해석 결과 — 위치와 함께 그 위치의 성격까지 한 번에 돌려준다.
 * (서브시스템 내부 계약)
 */
struct FLastFPSObjectiveGuidance
{
	FVector Location = FVector::ZeroVector;

	/**
	 * 동선 중간 지점이 아니라 최종 도달 지점인가.
	 * 중간 지점은 레벨에 이미 바닥으로 배치돼 있어 HUD 가 바닥 투영을 생략한다.
	 */
	bool bIsDestination = true;
};

/**
 * 표시 대상 목표 1건의 해석 결과 — 진행/요구량/완료 판정을 끝낸 상태로 방문자에게 넘긴다.
 * 목표 순회 규칙을 세 조회 함수가 각자 재작성하지 않게 하는 서브시스템 내부 계약이다.
 */
struct FLastFPSDisplayObjectiveEntry
{
	const FLastFPSQuestObjective* Objective = nullptr;
	int32 Index = INDEX_NONE;
	int32 Progress = 0;
	int32 RequiredCount = 1;
	bool bCompleted = false;
};

/**
 * 목표 1건의 표시 스냅샷.
 * 요구량 해석·완료 판정·안내 위치 해석을 서브시스템이 끝낸 결과만 담아, HUD 가 목표 판정 규칙을
 * 다시 구현하지 않게 한다. 거리(m)는 시청자 위치에 따라 바뀌므로 담지 않고 표시 측이 계산한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSTrackedObjective
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	ELastFPSObjectiveType Type = ELastFPSObjectiveType::AcquireItem;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	int32 Progress = 0;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	int32 RequiredCount = 1;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	bool bCompleted = false;

	/** 안내 위치가 존재하여 목표까지 거리 표시가 가능한지. */
	UPROPERTY(BlueprintReadOnly, Category="Quest")
	bool bHasGuidanceLocation = false;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FVector GuidanceLocation = FVector::ZeroVector;
};

/**
 * 진행중 퀘스트 1건의 표시 스냅샷 — 테이블 행 순서대로 반환된다.
 * 정렬 기준과 표시 개수 제한은 화면별 표시 정책이라 여기 담지 않는다.
 */
USTRUCT(BlueprintType)
struct FLastFPSTrackedQuest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	ELastFPSQuestType Type = ELastFPSQuestType::Main;

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	TArray<FLastFPSTrackedObjective> Objectives;
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
	TArray<int32> RefundedPurchaseQuantity; // 목표별 환급 대상으로 기록한 실제 구매 수량
	int32 EligiblePurchaseSpend = 0;        // 목표 수량 범위에서 발생한 실결제액 누계
};

USTRUCT(BlueprintType)
struct FLastFPSDungeonQuestMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSoftObjectPtr<UWorld> World;

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
	/** 월드 컨텍스트에서 서브시스템을 얻는 공용 접근자 (World→GameInstance 조회 중복 제거). */
	static ULastFPSQuestSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 런타임 상태 (없으면 NotStarted) */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	ELastFPSQuestStatus GetStatus(FName QuestId) const;

	/** 목표별 현재 진행 (인덱스 = 행 Objectives 순서). 범위 밖이면 0. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	int32 GetObjectiveProgress(FName QuestId, int32 ObjectiveIndex) const;

	/** 목표별 실제 요구량. ClearEncounter는 현재 Mode의 Encounter Profile을 기준으로 반환한다. */
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
	 * 퀘스트 취소 — 진행 중인 퀘스트를 초기화하고 미시작 상태로 되돌립니다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void CancelQuest(FName QuestId);

	/**
	 * 퀘스트 추적 설정 — 추적 대상은 항상 최대 1건이라 새로 추적하면 기존 추적을 대체한다.
	 * bTrack=true 는 진행중 퀘스트만 허용하고, false 는 지금 추적 중인 퀘스트일 때만 해제한다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void SetQuestTracked(FName QuestId, bool bTrack);

	/** 현재 퀘스트가 HUD 추적 활성화 상태인지 확인 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	bool IsQuestTracked(FName QuestId) const;

	/**
	 * 보상 수령 — Completed 일 때만 Claimed 로 1회 전이하며 Economy 에 크레딧/아이템 지급.
	 * 완료 전이거나 이미 수령했으면 false.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	bool TryClaimReward(FName QuestId);

#if !UE_BUILD_SHIPPING
	/**
	 * 개발용 — 대상 퀘스트에 도달하도록 선행 사슬을 전부 수령 완료로 만든다.
	 * 체인 뒷부분(던전 개방 등)을 앞 퀘스트를 다 깨지 않고 확인하기 위한 치트다.
	 * 보상 지급과 연출은 건너뛰고 상태만 진행시킨다.
	 */
	void DebugUnlockChainTo(FName QuestId);
#endif

	/** 정적 정의 접근 (UI 목록 생성용 — 테이블 단일 소스) */
	const UDataTable* GetQuestTable() const;
	const FLastFPSQuestData* FindQuest(FName QuestId) const;

	/** 퀘스트 제공 NPC의 표시 이름을 데이터 테이블에서 조회한다. 제공자가 없으면 빈 텍스트를 반환한다. */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	FText GetQuestGiverDisplayName(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	bool IsQuestMappedToCurrentMap(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	bool IsQuestMappedToAnyMap(FName QuestId) const;

	bool IsQuestInScopeForCurrentMap(FName QuestId) const;

	// ── 외부 이벤트 통지 (목표 진행 push) ────────────────────────────

	/** 처치 통지 — 서버 사망 브릿지가 오너 클라에서 호출. KillTarget 목표를 누적. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyObjectiveKill(FGameplayTag EnemyTag);

	/** 점령 완료 통지 — 월드 점령존이 오너 클라에서 호출. CaptureZone 목표를 누적. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyObjectiveCaptured(FGameplayTag ZoneTag);

	/** 방어 성공 통지 — 월드 방어존이 오너 클라에서 호출. DefendZone 목표를 누적. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyObjectiveDefended(FGameplayTag ZoneTag);

	/**
	 * 태그형 목표 완료 통지 — 유형을 데이터로 받는 공용 진입점.
	 * 목표 컴포넌트가 정의 에셋에서 받은 유형을 해석하지 않고 그대로 넘기는 데 쓴다.
	 * 유형별 처리는 트래커가 소유하므로 여기서 유형을 분기하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyTaggedObjective(ELastFPSObjectiveType Type, FGameplayTag Tag);

	/** 대화 통지 — 명시적으로 대화가 선택됐을 때 진행 중인 TalkToNPC 목표를 누적한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void NotifyTalkedToNPC(FName NPCRowName);

	/** 선택한 퀘스트 하나의 TalkToNPC 목표만 진행한다. NPC 퀘스트 행 클릭에서 사용한다. */
	bool NotifyTalkedToNPCForQuest(FName QuestId, FName NPCRowName);

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

	/**
	 * NPC 대화 데이터를 무전 자막으로 재생한다 — 대사 출력 채널을 자막 하나로 통일하기 위한 다리.
	 * 대화 테이블은 그대로 두고 표시만 무전 위젯을 쓰므로, 데이터 이관이 필요 없다.
	 * SpeakerName 이 비어 있으면 FallbackSpeaker(보통 NPC 표시 이름)를 쓴다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest|Radio")
	void TriggerDialogueAsRadio(
		const FLastFPSDialogueData& Dialogue,
		const FText& FallbackSpeaker,
		FLinearColor SpeakerColor);

	/**
	 * 이 NPC 의 임무 탭을 구성할 퀘스트 목록.
	 * OutAcceptable: 지금 이 NPC 에게서 수락할 수 있는 퀘스트, OutReportable: 이 NPC 에게 보고해야 하는 퀘스트.
	 * 표시 규칙(체인 잠금/맵 범위/순차 목표)은 마커와 같은 해석을 쓴다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void GetNPCQuestActions(FName NPCRowName, TArray<FName>& OutAcceptable, TArray<FName>& OutReportable) const;

	/** NPC 화면이 그대로 그릴 수 있는 퀘스트 클릭 행 스냅샷을 구성한다. */
	void GetNPCQuestOptions(FName NPCRowName, TArray<FLastFPSNPCQuestOption>& OutOptions) const;

	/** HUD 무전 위젯이 준비되기 전에 발생한 요청 중 가장 최신 묶음을 전달한다. */
	void FlushPendingRadioTransmissions();

	/** 라디오 송출을 즉시 모두 취소하고 자막을 지우도록 UI에 방송한다. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest|Radio")
	void StopAllRadioTransmissions();

	// ── 위치 마커 등록소 (ReachLocation 위치 소스 / HUD 공용) ─────────

	/** 위치 마커 등록 (ULastFPSObjectiveMarkerComponent 가 BeginPlay 에서 호출). */
	void RegisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker);
	/** 위치 마커 해제 (EndPlay). 같은 컴포넌트일 때만 제거. */
	void UnregisterLocationMarker(FGameplayTag LocationTag, USceneComponent* Marker);
	/** 태그에 등록된 월드 위치 조회. 없거나 파괴됐으면 false. */
	bool GetTrackedLocation(FGameplayTag LocationTag, FVector& OutLocation) const;

	/** 일반 RoomEncounter 밖에서 제공하는 Encounter 목표 위치를 등록한다. */
	void RegisterEncounterMarker(FName EncounterId, USceneComponent* Marker);
	/** 같은 컴포넌트가 등록한 Encounter 목표 위치만 해제한다. */
	void UnregisterEncounterMarker(FName EncounterId, USceneComponent* Marker);
	/** 등록된 Encounter 목표 위치를 조회한다. */
	bool GetTrackedEncounterLocation(FName EncounterId, FVector& OutLocation) const;

	/**
	 * NPC 1명의 마커 표시 상태 — 심볼(수락 가능/대화 목표)과 추적 강조 여부.
	 * 표시 규칙(순차 목표/맵 범위/체인 잠금)을 여기 한 곳에서만 해석해 마커 위젯과 HUD 가 공유한다.
	 *
	 * 레벨의 모든 NPC 마커가 상태 변경마다 같은 판정을 각자 반복하지 않도록, 계산은 상태가
	 * 바뀔 때 전체 NPC 분을 한 번에 하고 여기서는 그 결과만 돌려준다.
	 */
	UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
	FLastFPSQuestNPCMarkerInfo GetNPCMarkerInfo(FName NPCRowName) const;

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
	 * 추적 중인 퀘스트에서 안내 위치가 해석되는 미완료 목표를 반환한다.
	 * 어떤 유형이 위치를 갖는지는 ResolveObjectiveGuidanceLocation 이 단독으로 정하며,
	 * 여기서는 "위치가 나오면 웨이포인트"로만 판정한다.
	 * ReachLocation은 목적지 앞에 배치한 경로 지점을 진행 순서대로 먼저 안내한다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void GetActiveWaypoints(TArray<FLastFPSObjectiveWaypoint>& OutWaypoints) const;

	/**
	 * 진행중 퀘스트를 표시용 스냅샷으로 반환한다 — HUD 트래커/임무 화면 공용 읽기 계약.
	 * 순차 퀘스트는 완료한 단계와 현재 단계까지만 담아 다음 단계를 미리 공개하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Quest")
	void GetTrackedQuests(TArray<FLastFPSTrackedQuest>& OutQuests) const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest")
	FOnLastFPSQuestStateChanged OnQuestStateChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest|Radio")
	FOnLastFPSRadioTransmission OnRadioTransmission;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest|Radio")
	FOnLastFPSRadioTransmissionStop OnRadioTransmissionStop;

protected:
	/** 정확한 영속 월드와 자동 진행할 퀘스트 ID 매핑 (DefaultGame.ini 로 확장 가능) */
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

	/**
	 * 목표를 화면에서 안내할 지점 1개 — 화면 마커와 트래커 거리가 같은 지점을 가리키도록 공용화했다.
	 * ClearEncounter 는 등록된 인카운터 마커, ReachLocation 은 현재 동선 지점(없으면 도달 지점)을 쓴다.
	 * 그 밖의 유형은 false — 대화 목표는 NPC 머리 위 마커가, 아이템 획득 등은 트래커 목록이 안내한다.
	 */
	bool ResolveObjectiveGuidanceLocation(
		const FLastFPSQuestObjective& Objective,
		FLastFPSObjectiveGuidance& OutGuidance) const;

	/** 지금 추적 중인 퀘스트라면 추적을 비운다(체인 승계 전에 자리를 비우는 용도). 변경 시 true. */
	bool ClearTrackedQuestIfMatches(FName QuestId);

	/**
	 * 표시용으로 유효한 추적 퀘스트 1건(진행중 + 현재 맵 범위). 없으면 nullptr.
	 * 화면 마커·트래커·NPC 마커가 같은 전제를 각자 재작성하지 않도록 한 곳에 모았다.
	 */
	const FLastFPSQuestData* GetTrackedQuestForDisplay(
		FName& OutQuestId,
		const FLastFPSQuestRuntimeState*& OutState) const;

	/**
	 * 표시 대상 목표를 배열 순서대로 방문한다. Visitor 가 false 를 반환하면 중단하며,
	 * 순차 퀘스트는 첫 미완료 목표까지만 방문해 다음 단계를 미리 공개하지 않는다.
	 * (완료/요구량 해석과 순차 규칙의 단일 소스)
	 */
	void ForEachDisplayObjective(
		const FLastFPSQuestData& Def,
		const FLastFPSQuestRuntimeState& State,
		TFunctionRef<bool(const FLastFPSDisplayObjectiveEntry&)> Visitor) const;

	/** 추적/진행이 바뀐 뒤 NPC 마커 표시 캐시를 다시 계산한다(퀘스트 테이블 1회 순회). */
	void RebuildNPCMarkerCache();

	FDelegateHandle OnWorldInitHandle;
	void HandlePostWorldInitialization(const FActorsInitializedParams& Params);
	void BindEncounterEvents(UWorld& World);
	void UnbindEncounterEvents();

	/** 현재 영속 월드에 정확히 매핑된 던전 퀘스트를 1회 자동 수락한다. */
	void AcceptDungeonQuestForMap(UWorld& World);

	/**
	 * 반복 가능(bRepeatable) 임무가 이미 끝난 상태면 다시 수행할 수 있도록 초기화한다.
	 * 진행 중인 임무는 건드리지 않는다.
	 */
	void ResetRepeatableQuest(FName QuestId);

	/** 던전 퀘스트를 이미 자동 수락한 월드 (동일 월드 중복 처리 방지, 재입장 시 재수행 허용). */
	TWeakObjectPtr<UWorld> DungeonQuestAcceptedWorld;

	TArray<FName> CurrentMapQuestIds;

	/**
	 * 던전 퀘스트를 수락한 실시간 시각(초). 결과 화면의 클리어 시간 산출에만 쓴다.
	 * 음수면 측정 전이며, 결과 화면이 시간 표시를 접는다.
	 */
	double MissionStartRealTimeSeconds = -1.0;

	/** 클라이언트는 복제된 Encounter 진행 이벤트에서 Mode별 실제 요구량을 받는다. */
	TMap<FName, int32> EncounterRequiredCounts;

	/** 일반 RoomEncounter 밖에서 제공하는 Encounter 목표 위치다. */
	TMap<FName, TWeakObjectPtr<USceneComponent>> EncounterMarkers;

	/**
	 * 지금 추적 중인 퀘스트 1건. "동시에 하나"라는 규칙을 자료구조로 강제하려고
	 * 퀘스트별 플래그 대신 단일 값으로 소유한다. None 이면 추적 없음.
	 */
	FName TrackedQuestId;

	/** NPC 행 이름 → 마커 표시 상태 (상태 변경 시에만 재계산). 없는 키는 마커를 띄우지 않는다. */
	TMap<FName, FLastFPSQuestNPCMarkerInfo> NPCMarkerInfos;

	const UDataTable* GetRadioTable() const;
	const FLastFPSRadioTransmissionData* FindRadioTransmission(FName RadioId) const;

	/** 월드 초기화와 HUD 생성 사이에는 가장 최신 무전 요청만 보관한다. */
	TArray<FLastFPSRadioTransmissionData> PendingRadioTransmissions;

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

	/** 성공 구매 영수증을 진행 중 구매 퀘스트 한정으로 배분해 실결제액을 기록한다. */
	void HandlePurchaseCommitted(const FLastFPSPurchaseReceipt& Receipt);

	/** 한 퀘스트의 남은 AcquireItem 목표 수량에 영수증을 배분하고 소비한 수량을 반환한다. */
	int32 RecordPurchaseForQuest(
		FName QuestId,
		FLastFPSQuestRuntimeState& State,
		const FLastFPSQuestData& Def,
		const FLastFPSPurchaseReceipt& Receipt,
		int32 AvailableQuantity);

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
	bool ApplyEventToQuest(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def, const FLastFPSObjectiveEvent& Event);

	/**
	 * 모든 목표 충족 시 Completed 로 단조 승격. 승격했으면 true.
	 * 승격은 퀘스트당 한 번뿐이므로 완료 무전(RadioOnComplete)의 예약도 이 지점에서 함께 기록한다.
	 * 실제 재생은 보상 팝업이 닫힌 뒤 수행한다.
	 */
	bool CheckCompletion(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def);

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
	void GrantReward(FName QuestId, const FLastFPSQuestData& Def);

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

	/** 완료 팝업. 닫힌 뒤 OnClosed를 실행해 후속 대사와 퀘스트 전이를 잇는다. */
	void NotifyRewardGranted(
		FName QuestId,
		const FLastFPSQuestData& Def,
		int32 PurchaseRefundCredits,
		FSimpleDelegate OnClosed);

	/** 수령 알림 본문 — 제목 + 지급된 보상(크레딧/아이템) 내역. 구조화 보상이 비면 RewardText 폴백. */
	FText BuildRewardMessage(const FLastFPSQuestData& Def, int32 PurchaseRefundCredits) const;

	/** 현재 기록된 실결제액에 데이터 정책을 적용한 최종 환급액. */
	int32 ResolvePurchaseRefundCredits(FName QuestId, const FLastFPSQuestData& Def) const;

	TMap<FName, FLastFPSQuestRuntimeState> RuntimeStates;

	/** 정상 플레이 중 완료되어 보상 팝업 종료 뒤 재생해야 하는 퀘스트 완료 무전. */
	TSet<FName> PendingCompletionRadioQuestIds;

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
	bool bPurchaseSubscribed = false;
	TWeakObjectPtr<ULastFPSRoomEncounterSubsystem> BoundEncounterSubsystem;
};
