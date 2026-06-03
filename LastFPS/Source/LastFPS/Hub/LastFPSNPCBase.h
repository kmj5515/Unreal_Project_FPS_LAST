#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Hub/ILastFPSInteractable.h"
#include "LastFPSNPCBase.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UWidgetComponent;
class ULastFPSNPCMarkerWidget;

/**
 * 허브 월드 NPC 베이스
 * - 머리 위 3D 마커 (UWidgetComponent)
 * - 근접 감지 구체 (USphereComponent)
 * - 플레이어가 F키를 누르면 Interact() 호출
 * BP에서 상속해 메시 / 이름 / 역할 / 대화 내용 설정
 */
UCLASS(Blueprintable)
class LASTFPS_API ALastFPSNPCBase : public AActor, public ILastFPSInteractable
{
	GENERATED_BODY()

public:
	ALastFPSNPCBase();

	// ILastFPSInteractable
	virtual void Interact_Implementation(APlayerController* InstigatorPC) override;
	virtual FText GetInteractionLabel_Implementation() const override;

protected:
	virtual void BeginPlay() override;

	// ── 에디터 설정 ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText DisplayName = NSLOCTEXT("LastFPS", "NPC_DefaultName", "NPC");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText NPCRole = NSLOCTEXT("LastFPS", "NPC_DefaultRole", "");

	/** F키 힌트 텍스트. 기본값 "대화" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText InteractionLabel = NSLOCTEXT("LastFPS", "NPC_DefaultInteract", "대화");

	/** 근접 감지 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(ClampMin="50"))
	float InteractionRadius = 250.f;

	// ── 컴포넌트 ─────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 3D 플로팅 마커 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	TObjectPtr<UWidgetComponent> MarkerWidgetComp;

	/** Interact() 기본 구현: Notice 팝업 표시. BP에서 오버라이드해 대화/상점 등 연결 */
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
};
