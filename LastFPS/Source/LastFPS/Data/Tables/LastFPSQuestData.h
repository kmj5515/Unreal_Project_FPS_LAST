#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Cinematics/LastFPSCinematicTypes.h"
#include "LastFPSQuestData.generated.h"

class USoundBase;
class UTexture2D;

/** 무전 자막/사운드 팝업 연출 데이터 */
USTRUCT(BlueprintType)
struct FLastFPSRadioTransmissionData : public FTableRowBase
{
	GENERATED_BODY()

	/** 화자 이름 (예: "안내자", "HQ 사령관") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio")
	FText SpeakerName;

	/** 화자 하이라이트 컬러 (기본값: 노란색/골드) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio")
	FLinearColor SpeakerColor = FLinearColor(1.0f, 0.78f, 0.0f, 1.0f);

	/** 무전 대사 자막 내용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio", meta=(MultiLine=true))
	FText DialogueText;

	/** 함께 재생할 무전 음성 사운드 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio")
	TSoftObjectPtr<USoundBase> VoiceAudio;

	/** 자막 노출 유지 시간 (0 이면 기본 4.0초 적용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio", meta=(ClampMin=0.0, Units="s"))
	float DisplayDuration = 4.0f;

	/** 글자당 타이핑 속도(초) (0 이면 즉시 출력) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio", meta=(ClampMin=0.0, Units="s"))
	float TypingSpeed = 0.03f;
};

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
	ReachLocation	UMETA(DisplayName="위치 도달"),		// TargetTag = 위치 마커, TargetId(선택) = 연결할 EncounterId
	KillTarget		UMETA(DisplayName="대상 처치"),		// TargetTag = 적 종류, 서버 사망 이벤트로 카운트
	TalkToNPC		UMETA(DisplayName="NPC 대화"),		// TargetId = NPCRowName, 상호작용 완료로 카운트
	ClearEncounter	UMETA(DisplayName="인카운터 완료"),	// TargetId = EncounterId (DT_Encounter 행)
	CaptureZone		UMETA(DisplayName="구역 점령"),		// TargetTag = 점령 구역, 점령 완료 통지로 카운트
	DefendZone		UMETA(DisplayName="구역 방어")		// TargetTag = 방어 구역, 방어 성공 통지로 카운트
};

/**
 * 퀘스트 목표 1건.
 * - AcquireItem: TargetId(DT_ItemData 행)를 RequiredCount 개 획득 — 수락 시점 보유량을 기준선으로
 *   잡고 이후 증가분을 센다. 이미 갖고 있던 재고는 카운트하지 않음.
 * - ReachLocation: TargetTag 로 등록된 위치 마커에 AcceptRadius(m) 이내로 도달.
 *   던전 자동 트리거는 TargetId에 EncounterId를 지정해 해당 방의 트리거와 연결한다.
 * - KillTarget: TargetTag 종류의 적을 RequiredCount 기 처치(서버 사망 이벤트가 오너 클라로 통지).
 * - TalkToNPC: TargetId(NPCRowName) NPC 와 상호작용.
 * - ClearEncounter: TargetId(EncounterId) 방 인카운터 클리어.
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

	/** 행 참조형 대상 — AcquireItem=DT_ItemData 행, ReachLocation(선택)/ClearEncounter=EncounterId, TalkToNPC=NPCRowName */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FName TargetId;

	/** 분류형 대상 — KillTarget=적 종류 태그, ReachLocation=위치 마커 태그, CaptureZone/DefendZone=구역 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FGameplayTag TargetTag;

	/** 목표 수행 장소를 안내할 NPC 행. 비우면 목표 유형의 기본 대상만 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Guidance")
	FName GuidanceNPCRowName;

	/** 안내 NPC에서 목표 행을 선택했을 때 열 화면. 비우면 마커 안내만 수행한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Guidance")
	FGameplayTag GuidanceScreenTag;

	/** ReachLocation 도달 판정 반경(m). 다른 유형은 무시. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0.1, EditCondition="Type==ELastFPSObjectiveType::ReachLocation"))
	float AcceptRadius = 3.f;

	/** 필요 수량. ClearEncounter는 현재 Mode의 Encounter Profile에 정의된 적 수 합계를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=1))
	int32 RequiredCount = 1;

	/** 목록에 표시할 목표 문구 (예: "코어 3개 수집") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Label;

	/** 목표 시작 시 재생할 무전 대사 행 참조 (DT_RadioTransmission) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Radio")
	TArray<FName> RadioOnStart;


	/** 목표 완료 시 재생할 무전 대사 행 참조 (DT_RadioTransmission) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Radio")
	TArray<FName> RadioOnComplete;
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

/** 구매 목표의 실결제액 환급 정책. 환급 대상은 같은 퀘스트의 AcquireItem 목표가 결정한다. */
USTRUCT(BlueprintType)
struct FLastFPSQuestPurchaseRefund
{
	GENERATED_BODY()

	/** 실결제액 환급 비율. 0이면 환급을 사용하지 않고, 1이면 목표 수량의 실결제액을 전액 환급한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0.0, ClampMax=1.0))
	float Rate = 0.f;

	/** 퀘스트당 최대 환급액. 0이면 상한을 적용하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0))
	int32 MaxCredits = 0;

	[[nodiscard]] bool IsEnabled() const { return Rate > 0.f; }

	[[nodiscard]] int32 CalculateCredits(const int32 EligiblePurchaseSpend) const
	{
		if (!IsEnabled() || EligiblePurchaseSpend <= 0)
		{
			return 0;
		}

		const double SafeRate = static_cast<double>(FMath::Clamp(Rate, 0.f, 1.f));
		const int32 Calculated = FMath::Max(
			0,
			FMath::RoundToInt(static_cast<double>(EligiblePurchaseSpend) * SafeRate));
		return MaxCredits > 0 ? FMath::Min(Calculated, MaxCredits) : Calculated;
	}
};

/** 퀘스트 완료 보상 — 구매액 환급 + 고정 크레딧 + 아이템 목록. 수령 시 EconomySubsystem 으로 1회 지급. */
USTRUCT(BlueprintType)
struct FLastFPSQuestReward
{
	GENERATED_BODY()

	/** 환급과 별도로 지급하는 고정 완료 보너스 크레딧 (0이면 미지급) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(ClampMin=0))
	int32 Credits = 0;

	/** 수락 이후 AcquireItem 목표 아이템을 상점에서 구매한 실결제액 환급 정책 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FLastFPSQuestPurchaseRefund PurchaseRefund;

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

	/** 아코디언 메뉴의 그룹 제목 (예: "The Rohirrim") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	FText QuestGroupTitle;

	/** 아코디언 메뉴의 그룹 부제목 (예: "Chapter 1: Protection") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	FText QuestGroupSubtitle;

	/** 지역 이름 표시 (예: "Rohan") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	FText LocationName;

	/** 권장 레벨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	int32 RecommendedLevel = 1;

	/** 상세 화면에 표시될 가로 배너 이미지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	TSoftObjectPtr<UTexture2D> BannerImage;

	/**
	 * 초기 시드 상태 — 부팅 시 서브시스템이 이 값으로 런타임 상태를 시드한다.
	 * (예: InProgress = 시작부터 활성. 런타임 진행/완료/수령은 서브시스템이 관리하며 이 값을 덮지 않는다.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSQuestStatus Status = ELastFPSQuestStatus::NotStarted;

	/** 목록에 보일 한 줄 목표 요약 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Summary;

	/** NPC 상호작용 화면의 짧은 클릭 행 문구를 조회할 ST_Localize 키. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|UI")
	FName NPCButtonTextKey;

	/** 상세 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(MultiLine=true))
	FText Description;

	/** 달성 목표 목록 (AcquireItem 등). 비면 즉시 완료 가능한 퀘스트로 취급. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TArray<FLastFPSQuestObjective> Objectives;

	/** true면 배열 순서대로 목표 하나씩만 활성화한다. 던전의 이동→전투 단계처럼 순서가 있는 임무에 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	bool bSequentialObjectives = false;

	/** 퀘스트 시작 시 재생할 무전 대사 행 참조 (DT_RadioTransmission) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Radio")
	TArray<FName> RadioOnStart;

	/** 퀘스트 완료 시 재생할 무전 대사 행 참조 (DT_RadioTransmission) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Radio")
	TArray<FName> RadioOnComplete;

	/**
	 * 퀘스트 수락 시 재생할 컷신. Sequence 가 비면 연출 없이 진행한다.
	 * 무전(RadioOnStart)과 같은 시점에 걸리며, 재생 판정과 실제 재생은 컷신 재생 서브시스템이 소유한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Cinematic")
	FLastFPSCinematicPlayback CinematicOnStart;

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

	/**
	 * 완료한 뒤에도 다시 수행할 수 있는가.
	 * 퀘스트 상태는 원래 단조(NotStarted→…→Claimed)라 한 번 수령하면 끝난다. 던전처럼 같은 임무를
	 * 재입장할 때마다 새로 시작해야 하는 경우에만 켠다. 켜면 임무 시작 시점에 진행도가 초기화된다.
	 * 체인(PrereqQuestId/NextQuestId)과는 함께 쓰지 않는다 — 반복 퀘스트가 후속을 계속 다시 열게 된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest|Chain")
	bool bRepeatable = false;
};
