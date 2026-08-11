#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Hub/LastFPSNPCTypes.h"
#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSNPCQuestOption.h"
#include "LastFPSInteractionComponent.generated.h"

class APlayerController;
class USphereComponent;
class UWidgetComponent;
class UCameraComponent;
class ULastFPSNPCMarkerWidget;

/** 플레이어 근접 상태 변화 알림 (순찰 NPC가 정지/재개에 사용) */
DECLARE_MULTICAST_DELEGATE_OneParam(FLastFPSOnPlayerInRangeChanged, bool /*bInRange*/);

/**
 * NPC 상호작용 공용 컴포넌트.
 * - 설정(이름/역할/G힌트/반경/버튼 목록)을 한 곳에 보유
 * - 근접 감지(구체 오버랩), 머리 위 마커 갱신, 홀드 진행도 반영
 * - G 홀드 완료 시 퀘스트 용건이 있으면 허브를 열고, 없으면 Dialogue Data를 무전 HUD로 재생
 *
 * 씬 컴포넌트(구체/마커/카메라)는 소유 액터가 생성해 Setup()으로 전달한다.
 * (정지 NPC=AActor, 순찰 NPC=ACharacter 로 계층이 달라 액터가 소유하는 편이 자연스러움)
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULastFPSInteractionComponent();

	/** 소유 액터의 씬 컴포넌트를 연결하고 오버랩/마커를 초기화한다. BeginPlay에서 호출. */
	void Setup(USphereComponent* InSphere, UWidgetComponent* InMarker, UCameraComponent* InCamera);

	/**
	 * 데이터(DT_NPCData)로 설정을 주입한다. 스포너가 SpawnActorDeferred 후 FinishSpawning 전에 호출
	 * → BeginPlay의 Setup()이 이 값들로 마커를 초기화한다.
	 */
	void ApplyData(
		const FText& InName,
		const FText& InRole,
		const FText& InDescription,
		const FText& InLabel,
		float InRadius,
		const TArray<FLastFPSNPCAction>& InActions);

	/** 인터페이스 위임 대상 — 소유 액터의 Interact_Implementation에서 호출 */
	void HandleInteract(APlayerController* InstigatorPC);
	FText GetInteractionLabel() const { return InteractionLabel; }
	void SetProgress(float Progress);

	/** 플레이어 근접 상태 변화 (순찰 정지/재개용) */
	FLastFPSOnPlayerInRangeChanged OnPlayerInRangeChanged;

	// ── 설정 (NPC BP의 컴포넌트 Details에서 지정) ────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText DisplayName = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NPCDefaultName);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText NPCRole;

	/** NPC 상호작용 화면에 표시할 여러 줄 소개문. 비어 있으면 역할을 대체 문구로 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(MultiLine=true))
	FText NPCDescription;

	/** 머리 위 마커의 G 힌트 텍스트. 기본값 "대화" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC")
	FText InteractionLabel = FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NPCInteract);

	/** 근접 감지 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC", meta=(ClampMin="50"))
	float InteractionRadius = 150.f;

	/** 상호작용 허브의 버튼 목록. 비면 "대화 내용 없음" 공지로 폴백. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LastFPS|NPC")
	TArray<FLastFPSNPCAction> Actions;

	/** 기존 Dialogue Data를 무전 WBP로 표시할 때 적용할 화자 색상. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|NPC|Dialogue")
	FLinearColor DialogueRadioSpeakerColor = FLinearColor(0.65f, 0.85f, 1.f, 1.f);

private:
	/** 소유 NPC의 퀘스트 데이터 식별자를 얻는다. */
	FName ResolveQuestMarkerId() const;

	/** 현재 이 NPC가 표시할 퀘스트 행을 데이터 스냅샷으로 구성한다. */
	void BuildQuestOptions(TArray<FLastFPSNPCQuestOption>& OutOptions) const;

	/** 상점·모듈처럼 대화 외 화면으로 이동하는 유효한 액션이 있는지 확인한다. */
	bool HasServiceAction() const;

	/** 직접 재생 가능한 유효 대화 액션 수를 반환한다. */
	int32 CountValidDialogueActions() const;

	/** 첫 유효 Dialogue 액션을 기존 무전 WBP로 재생한다. */
	bool TryPlayIdleDialogue() const;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	static APlayerController* ResolveLocalPC(AActor* OtherActor);
	ULastFPSNPCMarkerWidget* GetMarkerWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> MarkerWidgetComp;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> TalkCamera;
};
