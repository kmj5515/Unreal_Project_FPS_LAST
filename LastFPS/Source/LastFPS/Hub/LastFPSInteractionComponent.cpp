#include "Hub/LastFPSInteractionComponent.h"

#include "Data/Tables/LastFPSDialogueData.h"
#include "Game/LastFPSPlayerController.h"
#include "Localization/LastFPSLocalization.h"
#include "Quest/LastFPSQuestMarkerTarget.h"
#include "Quest/LastFPSQuestSubsystem.h"
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
	const FText& InName,
	const FText& InRole,
	const FText& InDescription,
	const FText& InLabel,
	const float InRadius,
	const TArray<FLastFPSNPCAction>& InActions)
{
	DisplayName = InName;
	NPCRole = InRole;
	NPCDescription = InDescription;
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

	TArray<FLastFPSNPCQuestOption> QuestOptions;
	BuildQuestOptions(QuestOptions);
	const int32 DialogueCount = CountValidDialogueActions();

	// 퀘스트 행, 상점·모듈 서비스, 복수 대화 선택지가 있으면 선택 화면이 필요하다.
	if (!QuestOptions.IsEmpty() || HasServiceAction() || DialogueCount > 1)
	{
		PC->BeginNPCInteraction(
			GetOwner(),
			TalkCamera,
			DisplayName,
			NPCDescription.IsEmpty() ? NPCRole : NPCDescription,
			Actions,
			QuestOptions,
			DialogueRadioSpeakerColor);
		return;
	}

	// 선택할 것이 없는 단일 대화 NPC만 기존 Dialogue Data를 무전 WBP로 바로 재생한다.
	if (DialogueCount == 1 && TryPlayIdleDialogue())
	{
		return;
	}

	// 유효한 대화도 없으면 조용히 실패하지 않고 데이터 문제를 알린다.
	PC->ShowNotice(
		DisplayName,
		FText::Format(
			FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::NPCNoActions),
			DisplayName));
}

FName ULastFPSInteractionComponent::ResolveQuestMarkerId() const
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<ULastFPSQuestMarkerTarget>())
	{
		return ILastFPSQuestMarkerTarget::Execute_GetQuestMarkerId(Owner);
	}

	return NAME_None;
}

void ULastFPSInteractionComponent::BuildQuestOptions(TArray<FLastFPSNPCQuestOption>& OutOptions) const
{
	OutOptions.Reset();
	const FName MarkerId = ResolveQuestMarkerId();
	if (MarkerId.IsNone())
	{
		return;
	}

	const ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this);
	if (Quest == nullptr)
	{
		return;
	}

	Quest->GetNPCQuestOptions(MarkerId, OutOptions);
}

bool ULastFPSInteractionComponent::HasServiceAction() const
{
	return Actions.ContainsByPredicate(
		[](const FLastFPSNPCAction& Action)
		{
			return Action.Type == ELastFPSNPCActionType::Screen && Action.ScreenTag.IsValid();
		});
}

int32 ULastFPSInteractionComponent::CountValidDialogueActions() const
{
	int32 Count = 0;
	for (const FLastFPSNPCAction& Action : Actions)
	{
		if (Action.Type == ELastFPSNPCActionType::Dialogue
			&& Action.DialogueRow.DataTable != nullptr
			&& !Action.DialogueRow.RowName.IsNone())
		{
			++Count;
		}
	}
	return Count;
}

bool ULastFPSInteractionComponent::TryPlayIdleDialogue() const
{
	// 대화 액션이 여럿이면 어느 것을 들려줄지 고르는 것은 화면의 몫이라 첫 번째만 쓴다.
	const FLastFPSNPCAction* DialogueAction = Actions.FindByPredicate(
		[](const FLastFPSNPCAction& Action)
		{
			return Action.Type == ELastFPSNPCActionType::Dialogue
				&& Action.DialogueRow.DataTable != nullptr
				&& !Action.DialogueRow.RowName.IsNone();
		});

	if (DialogueAction == nullptr)
	{
		return false;
	}

	static const FString Ctx(TEXT("ULastFPSInteractionComponent::TryPlayIdleDialogue"));
	const FLastFPSDialogueData* Dialogue =
		DialogueAction->DialogueRow.DataTable->FindRow<FLastFPSDialogueData>(
			DialogueAction->DialogueRow.RowName, Ctx, /*bWarnIfMissing=*/false);
	if (Dialogue == nullptr || Dialogue->Lines.IsEmpty())
	{
		return false;
	}

	ULastFPSQuestSubsystem* Quest = ULastFPSQuestSubsystem::Get(this);
	if (Quest == nullptr)
	{
		return false;
	}

	Quest->TriggerDialogueAsRadio(*Dialogue, DisplayName, DialogueRadioSpeakerColor);
	return true;
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
