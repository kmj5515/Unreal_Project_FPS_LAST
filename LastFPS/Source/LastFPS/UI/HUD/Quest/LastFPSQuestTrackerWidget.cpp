#include "UI/HUD/Quest/LastFPSQuestTrackerWidget.h"

#include "Localization/LastFPSLocalization.h"
#include "UI/HUD/Quest/LastFPSQuestTrackerCardWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSQuestTracker, Log, All);

void ULastFPSQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem())
	{
		Subsystem->OnQuestStateChanged.AddUniqueDynamic(this, &ULastFPSQuestTrackerWidget::HandleQuestStateChanged);
	}

	// 빈 상태 문구는 변하지 않으므로 갱신마다 다시 만들지 않고 여기서 한 번만 채운다.
	if (TB_Empty)
	{
		TB_Empty->SetText(FLastFPSLocalization::GetUIText(LastFPSUIStringKeys::QuestTrackerEmpty));
	}

	RefreshTracker();
}

void ULastFPSQuestTrackerWidget::NativeDestruct()
{
	if (ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem())
	{
		Subsystem->OnQuestStateChanged.RemoveDynamic(this, &ULastFPSQuestTrackerWidget::HandleQuestStateChanged);
	}

	UpdateDistanceRefreshTimer(false);

	Super::NativeDestruct();
}

ULastFPSQuestSubsystem* ULastFPSQuestTrackerWidget::GetQuestSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<ULastFPSQuestSubsystem>();
		}
	}
	return nullptr;
}

void ULastFPSQuestTrackerWidget::HandleQuestStateChanged()
{
	RefreshTracker();
}

void ULastFPSQuestTrackerWidget::RefreshTracker()
{
	if (!Box_TrackerList)
	{
		return;
	}

	ULastFPSQuestSubsystem* Subsystem = GetQuestSubsystem();
	if (Subsystem)
	{
		Subsystem->GetTrackedQuests(TrackedQuests);
	}
	else
	{
		TrackedQuests.Reset();
	}

	// 메인 퀘스트를 위로 올리되 같은 분류 안에서는 테이블 행 순서를 지켜 목록이 튀지 않게 한다.
	TrackedQuests.StableSort([](const FLastFPSTrackedQuest& Lhs, const FLastFPSTrackedQuest& Rhs)
	{
		return Lhs.Type < Rhs.Type;
	});

	const FLastFPSQuestTrackerViewer Viewer = ResolveViewer();
	const int32 CardCount = FMath::Min(TrackedQuests.Num(), MaxTrackedQuests);
	bool bNeedsDistanceRefresh = false;

	for (int32 CardIndex = 0; CardIndex < CardCount; ++CardIndex)
	{
		ULastFPSQuestTrackerCardWidget* Card = AcquireQuestCard(CardIndex);
		if (!Card)
		{
			break;
		}

		const FLastFPSTrackedQuest& Quest = TrackedQuests[CardIndex];
		for (const FLastFPSTrackedObjective& Objective : Quest.Objectives)
		{
			bNeedsDistanceRefresh |= Objective.bHasGuidanceLocation;
		}

		Card->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Card->UpdateQuest(Quest, Viewer);
	}

	// 남는 카드는 파괴하지 않고 접어 둬서 다음 갱신에 그대로 재사용한다.
	for (int32 CardIndex = CardCount; CardIndex < QuestCards.Num(); ++CardIndex)
	{
		if (QuestCards[CardIndex])
		{
			QuestCards[CardIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (TB_Empty)
	{
		TB_Empty->SetVisibility(CardCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	UpdateDistanceRefreshTimer(bNeedsDistanceRefresh);
}

ULastFPSQuestTrackerCardWidget* ULastFPSQuestTrackerWidget::AcquireQuestCard(const int32 CardIndex)
{
	if (QuestCards.IsValidIndex(CardIndex))
	{
		return QuestCards[CardIndex];
	}

	if (!Box_TrackerList)
	{
		return nullptr;
	}

	if (!QuestCardWidgetClass)
	{
		if (!bQuestCardClassWarningLogged)
		{
			bQuestCardClassWarningLogged = true;
			UE_LOG(LogLastFPSQuestTracker, Warning,
				TEXT("[QuestTracker] %s: 퀘스트 카드를 만들지 못했습니다. WBP 의 QuestCardWidgetClass 에 WBP_QuestTrackerCard 가 지정됐는지 확인하세요."),
				*GetName());
		}
		return nullptr;
	}

	ULastFPSQuestTrackerCardWidget* Card =
		CreateWidget<ULastFPSQuestTrackerCardWidget>(this, QuestCardWidgetClass);
	if (!Card)
	{
		return nullptr;
	}

	Box_TrackerList->AddChild(Card);
	QuestCards.Add(Card);
	return Card;
}

FLastFPSQuestTrackerViewer ULastFPSQuestTrackerWidget::ResolveViewer() const
{
	FLastFPSQuestTrackerViewer Viewer;

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return Viewer;
	}

	if (const APawn* Pawn = PlayerController->GetPawn())
	{
		Viewer.Location = Pawn->GetActorLocation();
		Viewer.bValid = true;
		return Viewer;
	}

	// 관전/사망 등 폰이 없는 구간에서는 카메라 시점을 기준으로 거리를 보여 준다.
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(Viewer.Location, ViewRotation);
	Viewer.bValid = true;
	return Viewer;
}

void ULastFPSQuestTrackerWidget::UpdateDistanceRefreshTimer(const bool bNeedsDistanceRefresh)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	const bool bActive = TimerManager.IsTimerActive(DistanceRefreshTimerHandle);

	if (bNeedsDistanceRefresh && !bActive)
	{
		TimerManager.SetTimer(
			DistanceRefreshTimerHandle, this, &ULastFPSQuestTrackerWidget::RefreshTracker,
			DistanceRefreshInterval, /*bLoop=*/true);
	}
	else if (!bNeedsDistanceRefresh && bActive)
	{
		TimerManager.ClearTimer(DistanceRefreshTimerHandle);
	}
}
