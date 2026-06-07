#include "Hub/LastFPSNPCBase.h"

#include "Game/LastFPSPlayerController.h"
#include "Hub/LastFPSDialogueData.h"
#include "UI/LastFPSNPCMarkerWidget.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

ALastFPSNPCBase::ALastFPSNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->SetCapsuleHalfHeight(88.f);
	CapsuleComp->SetCapsuleRadius(34.f);
	CapsuleComp->SetCollisionProfileName(TEXT("BlockAll"));
	SetRootComponent(CapsuleComp);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetupAttachment(RootComponent);

	MarkerWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComp"));
	MarkerWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);   // 항상 카메라를 향함
	MarkerWidgetComp->SetDrawSize(FVector2D(200.f, 80.f));
	MarkerWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 머리 위
	MarkerWidgetComp->SetupAttachment(RootComponent);
}

void ALastFPSNPCBase::BeginPlay()
{
	Super::BeginPlay();

	// 구체 반경을 에디터 설정값으로 갱신
	InteractionSphere->SetSphereRadius(InteractionRadius);

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALastFPSNPCBase::HandleBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ALastFPSNPCBase::HandleEndOverlap);

	// 마커 위젯 초기화
	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetNPCInfo(DisplayName, NPCRole);
		Marker->SetInteractionLabel(InteractionLabel);
		Marker->SetInteractionHintVisible(false);
	}
}

// ── ILastFPSInteractable ─────────────────────────────────────────

void ALastFPSNPCBase::Interact_Implementation(APlayerController* InstigatorPC)
{
	OnInteract(InstigatorPC);
}

FText ALastFPSNPCBase::GetInteractionLabel_Implementation() const
{
	return InteractionLabel;
}

// ── 기본 상호작용 — BP에서 오버라이드 ───────────────────────────

void ALastFPSNPCBase::OnInteract_Implementation(APlayerController* InstigatorPC)
{
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(InstigatorPC);
	if (!PC)
	{
		return;
	}

	// 1) 화면이 지정 + 레지스트리에 등록돼 있으면 그 화면을 연다 (상점/임무 NPC 등).
	if (ScreenToOpen.IsValid() && PC->OpenScreen(ScreenToOpen))
	{
		return;
	}

	// 2) 대화 행이 지정돼 있으면 단방향 대화창을 띄운다 (일반 NPC).
	const FLastFPSDialogueData* Dialogue = DialogueRow.IsNull()
		? nullptr
		: DialogueRow.GetRow<FLastFPSDialogueData>(TEXT("NPC OnInteract"));
	if (Dialogue)
	{
		// 행에 화자 이름이 없으면 NPC의 DisplayName을 사용.
		const FText& Speaker = Dialogue->SpeakerName.IsEmpty() ? DisplayName : Dialogue->SpeakerName;
		PC->ShowDialogue(Speaker, Dialogue->Lines);
		return;
	}

	// 3) 화면·대화 모두 미지정 → 공지로 폴백 (조용한 실패 방지).
	PC->ShowNotice(
		DisplayName,
		FText::Format(
			NSLOCTEXT("LastFPS", "NPC_DefaultDialog", "{0}(와)과 대화 내용이 아직 없습니다."),
			DisplayName));
}

// ── 범위 감지 ────────────────────────────────────────────────────

void ALastFPSNPCBase::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ALastFPSPlayerController* PC = OtherActor
		? Cast<ALastFPSPlayerController>(OtherActor->GetInstigatorController())
		: nullptr;

	if (!PC)
	{
		if (APawn* Pawn = Cast<APawn>(OtherActor))
		{
			PC = Cast<ALastFPSPlayerController>(Pawn->GetController());
		}
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
}

void ALastFPSNPCBase::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
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
}

ULastFPSNPCMarkerWidget* ALastFPSNPCBase::GetMarkerWidget() const
{
	return MarkerWidgetComp
		? Cast<ULastFPSNPCMarkerWidget>(MarkerWidgetComp->GetUserWidgetObject())
		: nullptr;
}
