#include "Hub/LastFPSNPC.h"

#include "Hub/LastFPSInteractionComponent.h"
#include "Hub/LastFPSNPCSettings.h"
#include "Hub/LastFPSNPCSpawnData.h"
#include "Hub/LastFPSPatrolAIController.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSNPC, Log, All);

ALastFPSNPC::ALastFPSNPC()
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

	// 순찰용 AIController. 정지 NPC는 BeginPlay에서 스폰하지 않는다.
	AIControllerClass = ALastFPSPatrolAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;

	// 상호작용 능력
	InteractionComp = CreateDefaultSubobject<ULastFPSInteractionComponent>(TEXT("InteractionComp"));

	// 근접 감지 구체
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetupAttachment(RootComponent);

	// 머리 위 마커
	MarkerWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComp"));
	MarkerWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	MarkerWidgetComp->SetDrawSize(FVector2D(200.f, 80.f));
	MarkerWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	MarkerWidgetComp->SetupAttachment(RootComponent);

	// 대화 구도 카메라 — 기본은 NPC 정면 앞쪽에서 NPC를 바라보게. BP에서 조정.
	TalkCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TalkCamera"));
	TalkCamera->SetupAttachment(RootComponent);
	TalkCamera->SetRelativeLocation(FVector(180.f, 0.f, 60.f));
	TalkCamera->SetRelativeRotation(FRotator(0.f, 180.f, 0.f)); // NPC를 향해 뒤돌아봄
	TalkCamera->bUsePawnControlRotation = false;
}

void ALastFPSNPC::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionComp)
	{
		// JSON 데이터로 자기 설정 (Setup 전에 주입해야 마커가 올바른 값으로 초기화됨)
		ApplyDataFromTable();

		// 마커 위젯 클래스가 BP에 지정 안 됐으면 프로젝트 설정의 공통 마커를 사용
		if (MarkerWidgetComp && !MarkerWidgetComp->GetWidgetClass())
		{
			if (const ULastFPSNPCSettings* NPCSettings = GetDefault<ULastFPSNPCSettings>())
			{
				if (UClass* MarkerCls = NPCSettings->MarkerWidgetClass.LoadSynchronous())
				{
					MarkerWidgetComp->SetWidgetClass(MarkerCls);
					MarkerWidgetComp->InitWidget();
				}
			}
		}

		InteractionComp->OnPlayerInRangeChanged.AddUObject(this, &ALastFPSNPC::HandlePlayerInRangeChanged);
		InteractionComp->Setup(InteractionSphere, MarkerWidgetComp, TalkCamera);
	}

	// 순찰 지점이 있으면 AIController를 스폰해 BehaviorTree 순찰 시작.
	if (PatrolPoints.Num() > 0)
	{
		SpawnDefaultController();
	}
}

// ── ILastFPSInteractable → 컴포넌트 위임 ──────────────────────────────

void ALastFPSNPC::Interact_Implementation(APlayerController* InstigatorPC)
{
	if (InteractionComp)
	{
		InteractionComp->HandleInteract(InstigatorPC);
	}
}

FText ALastFPSNPC::GetInteractionLabel_Implementation() const
{
	return InteractionComp ? InteractionComp->GetInteractionLabel() : FText::GetEmpty();
}

void ALastFPSNPC::SetInteractionProgress_Implementation(float Progress)
{
	if (InteractionComp)
	{
		InteractionComp->SetProgress(Progress);
	}
}

// ── JSON 자기 설정 ────────────────────────────────────────────────────

void ALastFPSNPC::ApplyDataFromTable()
{
	if (NPCRowName.IsNone() || !InteractionComp)
	{
		return; // 행 미지정 → BP/컴포넌트에 직접 설정한 값 사용
	}

	const ULastFPSNPCSettings* Settings = GetDefault<ULastFPSNPCSettings>();
	UDataTable* NPCTable = Settings ? Settings->NPCDataTable.LoadSynchronous() : nullptr;
	if (!NPCTable)
	{
		UE_LOG(LogLastFPSNPC, Warning,
			TEXT("'%s': NPCDataTable 미설정. Project Settings → Game → LastFPS NPC 에서 지정하세요."),
			*GetName());
		return;
	}

	const FLastFPSNPCSpawnData* Row = NPCTable->FindRow<FLastFPSNPCSpawnData>(NPCRowName, TEXT("ALastFPSNPC"));
	if (!Row)
	{
		UE_LOG(LogLastFPSNPC, Warning, TEXT("'%s': DT_NPCData 에 행 '%s' 이 없습니다."),
			*GetName(), *NPCRowName.ToString());
		return;
	}

	UDataTable* DialogueTable = Settings ? Settings->DialogueTable.LoadSynchronous() : nullptr;
	InteractionComp->ApplyData(
		Row->DisplayName, Row->NPCRole, Row->InteractionLabel, Row->InteractionRadius,
		Row->BuildRuntimeActions(DialogueTable));
}

// ── 순찰 정지/재개 ────────────────────────────────────────────────────

void ALastFPSNPC::HandlePlayerInRangeChanged(bool bInRange)
{
	SetPatrolPaused(bInRange); // 근접 → 멈추고 응대, 이탈 → 재개
}

void ALastFPSNPC::SetPatrolPaused(bool bPaused)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return; // 정지 NPC (컨트롤러 없음)
	}

	if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
	{
		BB->SetValueAsBool(TEXT("bIsInteracting"), bPaused);
	}

	if (bPaused)
	{
		AICon->StopMovement();
	}
}
