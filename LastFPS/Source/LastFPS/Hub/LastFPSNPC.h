#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Hub/ILastFPSInteractable.h"
#include "LastFPSNPC.generated.h"

class USphereComponent;
class UWidgetComponent;
class UCameraComponent;
class ULastFPSInteractionComponent;

/**
 * 통합 NPC — 상호작용 가능한 모든 NPC의 단일 베이스.
 * - 몸: ACharacter (메시/이동). PatrolPoints를 채우면 순찰, 비우면 정지.
 * - 상호작용: ULastFPSInteractionComponent (설정·근접·마커·허브)
 * - 대화 구도용 카메라: UCameraComponent (BP에서 배치 후 컴포넌트 TalkCamera에 연결)
 * BP에서 상속해 메시/카메라/InteractionComponent의 Actions를 설정한다.
 */
UCLASS(Blueprintable)
class LASTFPS_API ALastFPSNPC : public ACharacter, public ILastFPSInteractable
{
	GENERATED_BODY()

public:
	ALastFPSNPC();

	// ── ILastFPSInteractable (컴포넌트로 위임) ────────────────────────
	virtual void Interact_Implementation(APlayerController* InstigatorPC) override;
	virtual FText GetInteractionLabel_Implementation() const override;
	virtual void SetInteractionProgress_Implementation(float Progress) override;

	/** 상호작용 컴포넌트 접근 */
	UFUNCTION(BlueprintCallable, Category="LastFPS|NPC")
	ULastFPSInteractionComponent* GetInteractionComponent() const { return InteractionComp; }

	/**
	 * DT_NPCData 의 행 이름. 지정하면 BeginPlay 에 그 행으로 이름/역할/버튼을 자동 설정한다.
	 * (레벨에 배치한 NPC가 JSON으로 스스로 설정 — 배치는 눈으로, 데이터는 JSON)
	 * 비우면 컴포넌트에 직접 설정한 값을 사용.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FName NPCRowName;

	// ── 순찰 데이터 (AIController/BTTask가 읽음) ──────────────────────
	/** 순찰 지점들 — 비우면 정지 NPC. 채우면 BeginPlay에서 AIController 스폰 후 순찰. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="LastFPS|Patrol")
	TArray<TObjectPtr<AActor>> PatrolPoints;

	/** 지점 도착 후 대기 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Patrol")
	float WaitAtPointSec = 2.0f;

	/** BTTask가 갱신/조회하는 현재 순찰 인덱스 */
	UPROPERTY(BlueprintReadWrite, Category="LastFPS|Patrol")
	int32 CurrentPatrolIndex = 0;

protected:
	virtual void BeginPlay() override;

	/** 상호작용 능력 (설정/로직 보유) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<ULastFPSInteractionComponent> InteractionComp;

	/** 근접 감지 구체 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 머리 위 3D 마커 (WBP_NPCMarker 할당) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<UWidgetComponent> MarkerWidgetComp;

	/** 대화 시 시점 전환용 카메라. BP에서 대화 구도로 배치. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<UCameraComponent> TalkCamera;

private:
	/** NPCRowName 이 지정된 경우 DT_NPCData 에서 설정을 읽어 컴포넌트에 주입 (Setup 전 호출) */
	void ApplyDataFromTable();

	/** 컴포넌트의 근접 알림 → 순찰 정지/재개 */
	void HandlePlayerInRangeChanged(bool bInRange);

	/** 순찰 일시정지: Blackboard bIsInteracting 토글 + 정지 */
	void SetPatrolPaused(bool bPaused);
};
