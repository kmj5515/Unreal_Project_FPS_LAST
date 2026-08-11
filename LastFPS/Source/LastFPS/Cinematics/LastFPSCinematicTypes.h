#pragma once

#include "CoreMinimal.h"
#include "LastFPSCinematicTypes.generated.h"

class ULevelSequence;
class UWorld;

/**
 * 컷신 1건의 재생 계약 — "무엇을, 어떤 조건으로 재생할지"만 담는 불변 설정이다.
 * 재생 중 상태(플레이어, 경과 시간, 스킵 여부)는 여기 담지 않고 재생 서브시스템이 소유한다.
 *
 * 퀘스트·인카운터 등 요청 주체가 늘어나도 이 구조만 데이터로 들고 있으면 되므로,
 * 재생 시스템은 요청 주체가 무엇인지 알 필요가 없다.
 */
USTRUCT(BlueprintType)
struct FLastFPSCinematicPlayback
{
	GENERATED_BODY()

	/** 재생할 레벨 시퀀스. 비어 있으면 "연출 없음"으로 취급해 요청 자체를 건너뛴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic")
	TSoftObjectPtr<ULevelSequence> Sequence;

	/**
	 * 이 시퀀스를 재생할 맵. 레벨 배치 액터를 바인딩한 시퀀스는 해당 맵에서만 의미가 있어
	 * 다른 맵에서 요청되면 조용히 건너뛴다(빈 화면 대신 연출 생략). 비우면 맵을 가리지 않는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic")
	TSoftObjectPtr<UWorld> RequiredWorld;

	/** 재생 시작 전 지연(초). 화면 전환·수락 UI가 닫힐 시간을 벌 때 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic", meta=(ClampMin=0.0, Units="s"))
	float StartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic", meta=(ClampMin=0.01))
	float PlayRate = 1.f;

	/** 재생 중 HUD 를 숨긴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic|Presentation")
	bool bHideHUD = true;

	/** 재생 중 플레이어 폰을 숨긴다. 시퀀스가 별도 배우를 세우는 컷신에 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic|Presentation")
	bool bHidePlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic|Presentation")
	bool bDisableMovementInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic|Presentation")
	bool bDisableLookAtInput = true;

	/**
	 * 플레이어가 건너뛸 수 있는가. 실제 스킵 입력 배선은 화면 측 책임이며,
	 * 이 값은 재생 서브시스템이 스킵 요청을 받아들일지 판단하는 기준이다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cinematic|Presentation")
	bool bSkippable = true;

	bool IsValidRequest() const { return !Sequence.IsNull(); }
};
