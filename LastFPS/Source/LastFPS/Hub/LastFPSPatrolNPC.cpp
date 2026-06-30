#include "Hub/LastFPSPatrolNPC.h"

#include "Hub/LastFPSPatrolAIController.h"
#include "Game/LastFPSPlayerController.h"
#include "Data/Tables/LastFPSDialogueData.h"
#include "UI/HUD/LastFPSNPCMarkerWidget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ALastFPSPatrolNPC::ALastFPSPatrolNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	// 이동 방향으로 자연스럽게 회전 (컨트롤러 Yaw 미사용)
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 360.f, 0.f);
		Move->MaxWalkSpeed = 150.f; // 산책 속도
	}

	// AIController가 자동으로 빙의해 BehaviorTree 실행
	AIControllerClass = ALastFPSPatrolAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 근접 감지 구체
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetupAttachment(RootComponent);

	// 머리 위 마커
	MarkerWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComp"));
	MarkerWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	MarkerWidgetComp->SetDrawSize(FVector2D(200.f, 80.f));
	MarkerWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	MarkerWidgetComp->SetupAttachment(RootComponent);
}

void ALastFPSPatrolNPC::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALastFPSPatrolNPC::HandleBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ALastFPSPatrolNPC::HandleEndOverlap);

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetNPCInfo(DisplayName, NPCRole);
		Marker->SetInteractionLabel(InteractionLabel);
		Marker->SetInteractionHintVisible(false);
		Marker->SetInteractionProgress(0.f);
	}
}

// ── ILastFPSInteractable ─────────────────────────────────────────────

void ALastFPSPatrolNPC::Interact_Implementation(APlayerController* InstigatorPC)
{
	OnInteract(InstigatorPC);
}

FText ALastFPSPatrolNPC::GetInteractionLabel_Implementation() const
{
	return InteractionLabel;
}

void ALastFPSPatrolNPC::SetInteractionProgress_Implementation(float Progress)
{
	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionProgress(Progress);
	}
}

// ── 기본 상호작용 — 정지 NPC와 동일 (화면/대화/공지 폴백) ────────────

void ALastFPSPatrolNPC::OnInteract_Implementation(APlayerController* InstigatorPC)
{
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(InstigatorPC);
	if (!PC)
	{
		return;
	}

	if (ScreenToOpen.IsValid() && PC->OpenScreen(ScreenToOpen))
	{
		return;
	}

	const FLastFPSDialogueData* Dialogue = DialogueRow.IsNull()
		? nullptr
		: DialogueRow.GetRow<FLastFPSDialogueData>(TEXT("PatrolNPC OnInteract"));
	if (Dialogue)
	{
		const FText& Speaker = Dialogue->SpeakerName.IsEmpty() ? DisplayName : Dialogue->SpeakerName;
		PC->ShowDialogue(Speaker, Dialogue->Lines);
		return;
	}

	PC->ShowNotice(
		DisplayName,
		FText::Format(
			NSLOCTEXT("LastFPS", "PatrolNPC_DefaultDialog", "{0}(와)과 대화 내용이 아직 없습니다."),
			DisplayName));
}

// ── 범위 감지 + 순찰 일시정지 ────────────────────────────────────────

void ALastFPSPatrolNPC::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ALastFPSPlayerController* PC = nullptr;
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		PC = Cast<ALastFPSPlayerController>(Pawn->GetController());
	}

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	PC->SetNearestInteractable(this);

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionHintVisible(true);
	}

	SetPatrolPaused(true); // 플레이어 근접 → 순찰 멈추고 응대
}

void ALastFPSPatrolNPC::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	ALastFPSPlayerController* PC = nullptr;
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		PC = Cast<ALastFPSPlayerController>(Pawn->GetController());
	}

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	PC->ClearNearestInteractable(this);

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionHintVisible(false);
	}

	SetPatrolPaused(false); // 멀어짐 → 순찰 재개
}

ULastFPSNPCMarkerWidget* ALastFPSPatrolNPC::GetMarkerWidget() const
{
	return MarkerWidgetComp
		? Cast<ULastFPSNPCMarkerWidget>(MarkerWidgetComp->GetUserWidgetObject())
		: nullptr;
}

void ALastFPSPatrolNPC::SetPatrolPaused(bool bPaused)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
	{
		// BT의 [순찰] 시퀀스 데코레이터(bIsInteracting == false, Observer aborts=Self)가 중단/재개를 처리.
		BB->SetValueAsBool(TEXT("bIsInteracting"), bPaused);
	}

	if (bPaused)
	{
		AICon->StopMovement();
	}
}
