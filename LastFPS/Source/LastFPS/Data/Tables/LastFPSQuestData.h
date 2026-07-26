#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LastFPSQuestData.generated.h"

class UTexture2D;

/** 퀘스트 분류 — 목록에서 메인/서브 구분 표시용 */
UENUM(BlueprintType)
enum class ELastFPSQuestType : uint8
{
	Main	UMETA(DisplayName="메인"),
	Side	UMETA(DisplayName="서브")
};

/**
 * 퀘스트 진행 상태.
 * DataTable 행의 Status 필드는 "초기 시드 상태"로만 쓰이고, 실제 런타임 진행/완료/수령은
 * ULastFPSQuestSubsystem 이 단조(monotonic) 전이로 소유한다: (Locked→)NotStarted→InProgress→Completed→Claimed.
 * Locked 는 선행 퀘스트 미완료로 아직 수락 불가한 상태다(체인 게이팅).
 * 신규 값은 기존 DataTable 행의 직렬화 값 보존을 위해 끝에 추가한다.
 */
UENUM(BlueprintType)
enum class ELastFPSQuestStatus : uint8
{
	NotStarted	UMETA(DisplayName="미시작"),
	InProgress	UMETA(DisplayName="진행중"),
	Completed	UMETA(DisplayName="완료"),		// 목표 달성, 보상 미수령
	Claimed		UMETA(DisplayName="수령완료"),	// 보상 지급 완료 (재지급 방지 래치)
	Locked		UMETA(DisplayName="잠김")		// 선행 퀘스트 미완료 → 수락 불가
};

/**
 * 퀘스트 목표 종류. 각 유형의 판정 소스는 서브시스템의 목표 트래커가 소유한다(Phase 2).
 * 신규 값은 기존 DataTable 행의 직렬화 값 보존을 위해 끝에 추가한다.
 */
UENUM(BlueprintType)
enum class ELastFPSObjectiveType : uint8
{
	AcquireItem		UMETA(DisplayName="아이템 획득"),	// TargetId = DT_ItemData 행, 기준선 이후 증가분
	ReachLocation	UMETA(DisplayName="위치 도달"),		// TargetTag = 위치 마커, AcceptRadius 이내 도달
	KillTarget		UMETA(DisplayName="대상 처치"),		// TargetTag = 적 종류, 서버 사망 이벤트로 카운트
	TalkToNPC		UMETA(DisplayName="NPC 대화"),		// TargetId = NPCRowName, 상호작용 완료로 카운트
	CaptureZone		UMETA(DisplayName="구역 점령"),		// TargetTag = 점령 구역, 점령 완료 통지로 카운트
	DefendZone		UMETA(DisplayName="구역 방어")		// TargetTag = 방어 구역, 방어 성공 통지로 카운트
};

/**
 * 퀘스트 목표 1건.
 * - AcquireItem: TargetId(DT_ItemData 행)를 RequiredCount 개 획득 — 수락 시점 보유량을 기준선으로
 *   잡고 이후 증가분을 센다. 이미 갖고 있던 재고는 카운트하지 않음.
 * - ReachLocation: TargetTag 로 등록된 위치 마커에 AcceptRadius(m) 이내로 도달.
 * - KillTarget: TargetTag 종류의 적을 RequiredCount 기 처치(서버 사망 이벤트가 오너 클라로 통지).
 * - TalkToNPC: TargetId(NPCRowName) NPC 와 상호작용.
 * - CaptureZone: TargetTag 구역을 RequiredCount 곳 점령(월드 점령존이 완료 시 오너 클라로 통지).
 * - DefendZone: TargetTag 구역을 RequiredCount 곳 방어 성공(월드 방어존이 성공 시 오너 클라로 통지).
 * TargetId(행 참조형)와 TargetTag(분류형)는 유형에 따라 각각 사용한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSObjectiveType Type = ELastFPSObjectiveType::AcquireItem;

	/** 행 참조형 대상 — AcquireItem=DT_ItemData 행, TalkToNPC=NPCRowName */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName TargetId;

	/** 분류형 대상 — KillTarget=적 종류 태그, ReachLocation=위치 마커 태그, CaptureZone/DefendZone=구역 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FGameplayTag TargetTag;

	/** ReachLocation 도달 판정 반경(m). 다른 유형은 무시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0.1, EditCondition="Type==ELastFPSObjectiveType::ReachLocation"))
	float AcceptRadius = 3.f;

	/** 필요 수량 (ReachLocation/TalkToNPC 는 1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=1))
	int32 RequiredCount = 1;

	/** 목록에 표시할 목표 문구 (예: "코어 3개 수집") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Label;
};

/** 보상으로 지급할 아이템 1건 (DT_ItemData 행 + 수량). */
USTRUCT(BlueprintType)
struct FLastFPSItemGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName RowId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=1))
	int32 Count = 1;
};

/** 퀘스트 완료 보상 — 크레딧 + 아이템 목록. 수령 시 EconomySubsystem 으로 1회 지급. */
USTRUCT(BlueprintType)
struct FLastFPSQuestReward
{
	GENERATED_BODY()

	/** 지급 크레딧 (0 이면 미지급) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0))
	int32 Credits = 0;

	/** 지급 아이템 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FLastFPSItemGrant> Items;
};

/**
 * 퀘스트 1건의 데이터 — DataTable(DT_QuestData) 행.
 * 정적 정의(제목/목표/보상)만 담는다. 실제 진행/완료/수령 상태는 ULastFPSQuestSubsystem 이 런타임으로 소유.
 */
USTRUCT(BlueprintType)
struct FLastFPSQuestData : public FTableRowBase
{
	GENERATED_BODY()

	/** 목록/상세에 표시할 퀘스트 제목 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Title;

	/** 메인 / 서브 구분 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSQuestType Type = ELastFPSQuestType::Main;

	/**
	 * 초기 시드 상태 — 부팅 시 서브시스템이 이 값으로 런타임 상태를 시드한다.
	 * (예: InProgress = 시작부터 활성. 런타임 진행/완료/수령은 서브시스템이 관리하며 이 값을 덮지 않는다.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSQuestStatus Status = ELastFPSQuestStatus::NotStarted;

	/** 목록에 보일 한 줄 목표 요약 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Summary;

	/** 상세 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(MultiLine=true))
	FText Description;

	/** 달성 목표 목록 (AcquireItem 등). 비면 즉시 완료 가능한 퀘스트로 취급. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FLastFPSQuestObjective> Objectives;

	/** 완료 보상 (크레딧 + 아이템) — 수령 시 1회 지급 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FLastFPSQuestReward Reward;

	/** 보상 표시 문자열 (구조화 Reward 도입 전 폴백/자유표기용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText RewardText;

	/** 목록 아이콘 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 선행 퀘스트 (지정 시 이 퀘스트가 Claimed 여야 잠금 해제). 비면 처음부터 열림. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Chain")
	FName PrereqQuestId;

	/** 다음 퀘스트 (완료·수령 후 자동 잠금 해제/수락). 비면 체인 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Chain")
	FName NextQuestId;

	/** 수락 트리거 NPC (지정 시 이 NPC 와 대화해야 수락). 비면 잠금 해제 즉시 자동 수락(스토리 체인형). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Chain")
	FName QuestGiverNPC;

	/** 목표 달성 즉시 보상 자동 지급(Completed→Claimed). false 면 수동 Claim 버튼 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Chain")
	bool bAutoClaim = false;
};
