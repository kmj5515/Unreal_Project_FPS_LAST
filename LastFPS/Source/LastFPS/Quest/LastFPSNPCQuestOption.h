#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LastFPSNPCQuestOption.generated.h"

/** NPC 상호작용 화면에서 퀘스트 행을 눌렀을 때 수행할 동작. */
UENUM(BlueprintType)
enum class ELastFPSNPCQuestOptionType : uint8
{
	Accept UMETA(DisplayName="수락"),
	Report UMETA(DisplayName="보고"),
	OpenGuidanceScreen UMETA(DisplayName="연계 화면 열기")
};

/**
 * NPC 상호작용 화면에 전달하는 퀘스트 표시 스냅샷.
 * 위젯이 DataTable과 런타임 상태를 다시 해석하지 않도록 표시 데이터와 동작 식별자를 함께 전달한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSNPCQuestOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	FText Title;

	/** 레퍼런스 화면의 하단 클릭 행에 표시할 한 줄 내용. */
	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	FText Content;

	/** 상세 설명은 클릭 행의 툴팁 등 보조 표시에 사용한다. */
	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	ELastFPSNPCQuestOptionType Type = ELastFPSNPCQuestOptionType::Accept;

	/** OpenGuidanceScreen 동작이 열 화면. 다른 동작에서는 비어 있다. */
	UPROPERTY(BlueprintReadOnly, Category="LastFPS|Quest")
	FGameplayTag ScreenTag;
};
