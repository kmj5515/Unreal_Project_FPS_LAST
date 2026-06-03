#include "Hub/LastFPSNPCBase.h"

#include "Game/LastFPSPlayerController.h"
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
	if (ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(InstigatorPC))
	{
		PC->ShowNotice(
			DisplayName,
			FText::Format(
				NSLOCTEXT("LastFPS", "NPC_DefaultDialog", "{0}(와)과 대화하려면 대화 시스템을 구현해주세요."),
				DisplayName));
	}
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
