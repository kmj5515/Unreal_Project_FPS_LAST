#pragma once

#include "CoreMinimal.h"
#include "Data/Tables/LastFPSQuestData.h"
#include "LastFPSMissionResultTypes.generated.h"

class UTexture2D;

/**
 * 임무 결과 화면이 소비하는 전투 통계 원본값.
 *
 * 서식이 끝난 문자열이 아니라 원본 수치를 담는다. 라벨과 서식은 표시하는 위젯이 소유해야
 * 화면마다 다른 표기를 쓸 수 있고, 통계를 채우는 쪽(게임플레이)이 UI 문구를 알지 않아도 된다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSMissionCombatStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	float DamageDealt = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	float DamageTaken = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 Assists = 0;
};

/**
 * 임무 1회의 결과 — 결과 화면과 게임플레이 사이의 안정적인 데이터 계약.
 *
 * 화면은 이 구조체만 소비하고 QuestSubsystem·PlayerState·Economy 를 직접 알지 않는다.
 * 값이 없는 항목은 각 필드의 "미표시 조건"을 따라 위젯이 해당 구획을 접는다.
 */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSMissionResult
{
	GENERATED_BODY()

	/** 결과 화면 상단 제목 (임무·던전 이름) */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	FText MissionName;

	/** 클리어 소요 시간. 음수면 측정되지 않은 것으로 보고 표시하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	float ElapsedSeconds = -1.f;

	/** 지급된 크레딧. 0 이하면 표시하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 Credits = 0;

	/** 지급된 아이템. 비어 있으면 획득 목록을 접는다. */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	TArray<FLastFPSItemGrant> Items;

	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	FLastFPSMissionCombatStats CombatStats;

	/** 사용한 캐릭터 초상. 비어 있으면 초상 구획을 접는다. */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	TSoftObjectPtr<UTexture2D> CharacterPortrait;

	/**
	 * 임무 점수.
	 *
	 * 점수 산출 시스템이 아직 없어 현재는 항상 INDEX_NONE 이며 위젯이 점수 구획을 접는다.
	 * 시스템이 생기면 이 값을 채우는 쪽만 바뀌고 위젯과 WBP 는 그대로 둔다.
	 */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 Score = INDEX_NONE;

	/** 점수 게이지의 최대치. 0 이하면 게이지를 접는다. */
	UPROPERTY(BlueprintReadOnly, Category="MissionResult")
	int32 ScoreMax = 0;
};
