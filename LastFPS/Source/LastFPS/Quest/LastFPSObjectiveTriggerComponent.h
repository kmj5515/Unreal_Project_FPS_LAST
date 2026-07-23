#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"
#include "LastFPSObjectiveTriggerComponent.generated.h"

class ULastFPSQuestSubsystem;

/**
 * 퀘스트 위치 목표(ReachLocation)의 볼륨형 도달 판정 소스.
 * 반경 마커(ULastFPSObjectiveMarkerComponent)가 점+반경이라면, 이 컴포넌트는 임의 박스 볼륨으로
 * 로컬 플레이어가 안에 들어오면 도달로 간주한다. 판정은 오버랩(push)으로 즉시 통지해 폴 지연이 없다.
 * 같은 LocationTag 로 위치도 등록하므로 HUD 화면 마커가 이 박스를 가리킨다.
 * 목표와의 매칭은 LocationTag == 목표.TargetTag. 한 태그에는 반경/볼륨 중 하나만 배치한다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSObjectiveTriggerComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	ULastFPSObjectiveTriggerComponent();

	/** 이 볼륨을 식별하는 위치 태그 (목표의 TargetTag 와 매칭) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Quest")
	FGameplayTag LocationTag;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	ULastFPSQuestSubsystem* GetQuestSubsystem() const;

	/** 로컬에서 조종하는 플레이어 폰만 도달로 인정 (퀘스트는 로컬 판정). */
	static bool IsLocalPlayerPawn(const AActor* Actor);

	/** HUD 마커 위치 등록 여부 (EndPlay 해제용). */
	bool bRegistered = false;

	/** 로컬 플레이어가 현재 볼륨 안인지 — 여러 오버랩 쌍을 하나의 진입/퇴장으로 합친다. */
	bool bPlayerInside = false;
};
