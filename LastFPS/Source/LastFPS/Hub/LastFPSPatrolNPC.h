#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Hub/ILastFPSInteractable.h"
#include "LastFPSPatrolNPC.generated.h"

class USphereComponent;
class UWidgetComponent;
class ULastFPSNPCMarkerWidget;

/**
 * 순찰(이동) NPC — 정석 구조.
 * - Pawn(몸): ACharacter + CharacterMovement + 메시/AnimBP + 상호작용(구체·마커)
 * - Brain(뇌): ALastFPSPatrolAIController + BehaviorTree (별도 클래스)
 * 상호작용 의미는 정지 NPC(ALastFPSNPCBase)와 동일: ScreenToOpen → DialogueRow → 공지 폴백.
 * (정지 NPC와의 코드 공유는 추후 InteractionComponent로 추출 — 현재는 자기완결형)
 */
UCLASS(Blueprintable)
class LASTFPS_API ALastFPSPatrolNPC : public ACharacter, public ILastFPSInteractable
{
	GENERATED_BODY()

public:
	ALastFPSPatrolNPC();

	// ── ILastFPSInteractable ─────────────────────────────────────────
	virtual void Interact_Implementation(APlayerController* InstigatorPC) override;
	virtual FText GetInteractionLabel_Implementation() const override;
	virtual void SetInteractionProgress_Implementation(float Progress) override;

	// ── 순찰 데이터 (AIController/BTTask가 읽음) ──────────────────────
	/** 순찰 지점들 — 레벨의 TargetPoint(또는 임의 액터)를 인스턴스에서 연결 */
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

	// ── 상호작용 설정 (정지 NPC와 동일 의미) ─────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText DisplayName = NSLOCTEXT("LastFPS", "PatrolNPC_DefaultName", "NPC");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText NPCRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText InteractionLabel = NSLOCTEXT("LastFPS", "PatrolNPC_DefaultInteract", "대화");

	/** 지정 + 등록 시 그 화면을 연다(상점/임무 NPC). 비우면 대화로 폴백. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(Categories="UI.Screen"))
	FGameplayTag ScreenToOpen;

	/** ScreenToOpen 미지정 시 표시할 대화 행 (FLastFPSDialogueData). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(RowType="/Script/LastFPS.LastFPSDialogueData"))
	FDataTableRowHandle DialogueRow;

	/** 근접 감지 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(ClampMin="50"))
	float InteractionRadius = 150.f;

	// ── 컴포넌트 ─────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 3D 플로팅 마커 (WBP_NPCMarker 할당) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<UWidgetComponent> MarkerWidgetComp;

	/** Interact 기본 구현: 화면/대화/공지. BP에서 오버라이드 가능. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="LastFPS|NPC")
	void OnInteract(APlayerController* InstigatorPC);
	virtual void OnInteract_Implementation(APlayerController* InstigatorPC);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	ULastFPSNPCMarkerWidget* GetMarkerWidget() const;

	/** 대화/근접 중 순찰 일시정지 — AIController Blackboard의 bIsInteracting 토글 + 정지 */
	void SetPatrolPaused(bool bPaused);
};
