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

/** 퀘스트 진행 상태 — 프로토 단계에선 DataTable 행에 직접 명시 */
UENUM(BlueprintType)
enum class ELastFPSQuestStatus : uint8
{
	NotStarted	UMETA(DisplayName="미시작"),
	InProgress	UMETA(DisplayName="진행중"),
	Completed	UMETA(DisplayName="완료")
};

/**
 * 퀘스트 1건의 데이터 — DataTable 행.
 * 목록 UI(임무 화면)가 모든 행을 읽어 나열한다. 진행 추적 로직(서브시스템)은 아직 없고,
 * 상태는 행에 직접 적는 프로토 단계.
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

	/** 진행 상태 (프로토: 행에 직접 명시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	ELastFPSQuestStatus Status = ELastFPSQuestStatus::NotStarted;

	/** 목록에 보일 한 줄 목표 요약 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText Summary;

	/** 상세 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest", meta=(MultiLine=true))
	FText Description;

	/** 보상 표시 문자열 (아이템 연동 전 임시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText RewardText;

	/** 목록 아이콘 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	TSoftObjectPtr<UTexture2D> Icon;
};
