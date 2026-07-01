#include "Hub/LastFPSInteractionComponent.h"

#include "Game/LastFPSPlayerController.h"
#include "UI/HUD/LastFPSNPCMarkerWidget.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"

ULastFPSInteractionComponent::ULastFPSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULastFPSInteractionComponent::Setup(USphereComponent* InSphere, UWidgetComponent* InMarker, UCameraComponent* InCamera)
{
	InteractionSphere = InSphere;
	MarkerWidgetComp = InMarker;
	TalkCamera = InCamera;

	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractionRadius);
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ULastFPSInteractionComponent::HandleBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ULastFPSInteractionComponent::HandleEndOverlap);
	}

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetNPCInfo(DisplayName, NPCRole);
		Marker->SetInteractionLabel(InteractionLabel);
		Marker->SetInteractionHintVisible(false);
		Marker->SetInteractionProgress(0.f);
	}
}

void ULastFPSInteractionComponent::ApplyData(
	const FText& InName, const FText& InRole, const FText& InLabel, float InRadius, const TArray<FLastFPSNPCAction>& InActions)
{
	DisplayName = InName;
	NPCRole = InRole;
	InteractionLabel = InLabel;
	InteractionRadius = InRadius;
	Actions = InActions;
}

// ── 인터페이스 위임 ──────────────────────────────────────────────────

void ULastFPSInteractionComponent::HandleInteract(APlayerController* InstigatorPC)
{
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(InstigatorPC);
	if (!PC)
	{
		return;
	}

	// 액션이 하나도 없으면 조용한 실패 대신 공지로 알린다.
	if (Actions.Num() == 0)
	{
		PC->ShowNotice(
			DisplayName,
			FText::Format(
				NSLOCTEXT("LastFPS", "NPC_NoActions", "{0}(와)과 대화 내용이 아직 없습니다."),
				DisplayName));
		return;
	}

	// NPC 카메라로 전환 + 허브 메뉴 열기 (실제 시점/UI 처리는 PlayerController).
	PC->BeginNPCInteraction(GetOwner(), TalkCamera, DisplayName, NPCRole, Actions);
}

void ULastFPSInteractionComponent::SetProgress(float Progress)
{
	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionProgress(Progress);
	}
}

// ── 근접 감지 ────────────────────────────────────────────────────────

void ULastFPSInteractionComponent::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(ResolveLocalPC(OtherActor));
	if (!PC)
	{
		return;
	}

	PC->SetNearestInteractable(GetOwner());

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionHintVisible(true);
	}

	OnPlayerInRangeChanged.Broadcast(true);
}

void ULastFPSInteractionComponent::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	ALastFPSPlayerController* PC = Cast<ALastFPSPlayerController>(ResolveLocalPC(OtherActor));
	if (!PC)
	{
		return;
	}

	PC->ClearNearestInteractable(GetOwner());

	if (ULastFPSNPCMarkerWidget* Marker = GetMarkerWidget())
	{
		Marker->SetInteractionHintVisible(false);
	}

	OnPlayerInRangeChanged.Broadcast(false);
}

APlayerController* ULastFPSInteractionComponent::ResolveLocalPC(AActor* OtherActor)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return (PC && PC->IsLocalController()) ? PC : nullptr;
}

ULastFPSNPCMarkerWidget* ULastFPSInteractionComponent::GetMarkerWidget() const
{
	return MarkerWidgetComp
		? Cast<ULastFPSNPCMarkerWidget>(MarkerWidgetComp->GetUserWidgetObject())
		: nullptr;
}
