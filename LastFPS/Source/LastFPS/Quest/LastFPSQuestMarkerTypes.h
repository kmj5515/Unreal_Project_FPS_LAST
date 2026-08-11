#pragma once

#include "CoreMinimal.h"
#include "LastFPSQuestMarkerTypes.generated.h"

/**
 * NPC 머리 위 마커에 띄울 심볼. 표시 규칙을 위젯이 다시 해석하지 않도록 서브시스템이 결정한다.
 * 신규 값은 기존 직렬화 값 보존을 위해 끝에 추가한다.
 */
UENUM(BlueprintType)
enum class ELastFPSQuestNPCMarkerSymbol : uint8
{
	None		UMETA(DisplayName="없음"),		// 대응할 퀘스트가 없어 마커를 띄우지 않는다
	Available	UMETA(DisplayName="수락 가능"),	// 이 NPC 가 아직 수락하지 않은 퀘스트를 준다 (느낌표)
	Objective	UMETA(DisplayName="대화 목표"),	// 진행중 퀘스트의 대화 목표 대상이다 (물음표)
	InProgress	UMETA(DisplayName="진행 중")		// 대화 목표가 없는 진행 중 퀘스트의 의뢰인이다
};

/**
 * NPC 1명의 마커 표시 상태 — 심볼과 강조(추적 중) 여부.
 * 표시 계약만 담아 UI 가 퀘스트 서브시스템 헤더 전체에 의존하지 않게 분리했다.
 */
USTRUCT(BlueprintType)
struct FLastFPSQuestNPCMarkerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Quest")
	ELastFPSQuestNPCMarkerSymbol Symbol = ELastFPSQuestNPCMarkerSymbol::None;

	/** 이 마커가 지금 추적 중인 퀘스트에 속하는가 (강조 색 적용 기준). */
	UPROPERTY(BlueprintReadOnly, Category="Quest")
	bool bTracked = false;

	/** 표시 상태가 실제로 바뀌었을 때만 위젯을 갱신하기 위한 비교. */
	bool operator==(const FLastFPSQuestNPCMarkerInfo& Other) const
	{
		return Symbol == Other.Symbol && bTracked == Other.bTracked;
	}
};
