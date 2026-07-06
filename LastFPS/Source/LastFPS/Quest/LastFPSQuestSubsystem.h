#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSQuestSubsystem.generated.h"

class UDataTable;
class ULastFPSEconomySubsystem;

/** 퀘스트 상태/진행이 바뀔 때 브로드캐스트 — 임무 화면/트래커 UI 갱신용 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSQuestStateChanged);

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

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Quest")
	FOnLastFPSQuestStateChanged OnQuestStateChanged;

protected:
	/** 퀘스트 정의 테이블 (DT_QuestData) — DefaultGame.ini 로 지정 */
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Quest")
	TSoftObjectPtr<UDataTable> QuestTable;

private:
	ULastFPSEconomySubsystem* GetEconomy() const;

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

	/** 한 퀘스트의 진행을 절대상태에서 재계산(기준선 차감). 완료 도달 시 Completed 로 단조 승격. 변경 시 true. */
	bool RecomputeProgress(FName QuestId, FLastFPSQuestRuntimeState& State, const FLastFPSQuestData& Def);

	/** 목표별 기준선 캡처 (AcquireItem = 현재 보유량). */
	void CaptureBaseline(const FLastFPSQuestData& Def, FLastFPSQuestRuntimeState& State) const;

	/** 완료 토스트 (로컬 PC 의 ShowNotice). */
	void NotifyRewardGranted(const FLastFPSQuestData& Def) const;

	/** 수령 알림 본문 — 제목 + 지급된 보상(크레딧/아이템) 내역. 구조화 보상이 비면 RewardText 폴백. */
	FText BuildRewardMessage(const FLastFPSQuestData& Def) const;

	TMap<FName, FLastFPSQuestRuntimeState> RuntimeStates;
	bool bInventorySubscribed = false;
};
