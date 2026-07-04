#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
 * ULastFPSQuestSubsystem 이 단조(monotonic) 전이로 소유한다: NotStarted→InProgress→Completed→Claimed.
 */
UENUM(BlueprintType)
enum class ELastFPSQuestStatus : uint8
{
	NotStarted	UMETA(DisplayName="미시작"),
	InProgress	UMETA(DisplayName="진행중"),
	Completed	UMETA(DisplayName="완료"),		// 목표 달성, 보상 미수령
	Claimed		UMETA(DisplayName="수령완료")	// 보상 지급 완료 (재지급 방지 래치)
};

/** 퀘스트 목표 종류. 현재 AcquireItem 하나(추후 TalkToNPC/ReachCredits 확장 여지). */
UENUM(BlueprintType)
enum class ELastFPSObjectiveType : uint8
{
	AcquireItem	UMETA(DisplayName="아이템 획득")
};

/**
 * 퀘스트 목표 1건.
 * AcquireItem: TargetId(DT_ItemData 행) 아이템을 RequiredCount 개 "획득" — 수락 시점 보유량을
 * 기준선으로 잡고 그 이후 증가분으로 진행을 센다(서브시스템). 이미 갖고 있던 재고는 카운트하지 않음.
 */
USTRUCT(BlueprintType)
struct FLastFPSQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSObjectiveType Type = ELastFPSObjectiveType::AcquireItem;

	/** 대상 식별자 — AcquireItem 이면 DT_ItemData 행 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName TargetId;

	/** 필요 수량 */
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
};
