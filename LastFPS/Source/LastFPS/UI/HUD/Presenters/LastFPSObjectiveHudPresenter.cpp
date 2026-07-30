#include "UI/HUD/Presenters/LastFPSObjectiveHudPresenter.h"

#include "Components/Widget.h"
#include "Encounter/LastFPSTimedObjectiveComponent.h"
#include "Engine/World.h"
#include "UI/HUD/LastFPSCaptureObjectiveWidget.h"
#include "UI/HUD/LastFPSDefendObjectiveWidget.h"
#include "UI/HUD/LastFPSObjectiveHudSubsystem.h"

void ULastFPSObjectiveHudPresenter::Initialize(
	ULastFPSDefendObjectiveWidget* InDefendWidget,
	ULastFPSCaptureObjectiveWidget* InCaptureWidget)
{
	DefendWidget = InDefendWidget;
	CaptureWidget = InCaptureWidget;

	ApplyMode(ELastFPSObjectiveHudMode::None);
}

void ULastFPSObjectiveHudPresenter::BindToWorld(UWorld* World)
{
	if (!World || BoundWorld.Get() == World)
	{
		return;
	}

	Unbind();

	ULastFPSObjectiveHudSubsystem* Hud = World->GetSubsystem<ULastFPSObjectiveHudSubsystem>();
	if (!Hud)
	{
		return;
	}

	Hud->OnPresentationChanged.AddUniqueDynamic(
		this, &ULastFPSObjectiveHudPresenter::HandlePresentationChanged);
	BoundWorld = World;

	// HUD 가 목표보다 늦게 만들어졌을 수 있으므로 현재 상태를 즉시 반영한다.
	HandlePresentationChanged(Hud->GetActiveMode(), Hud->GetActiveSource());
}

void ULastFPSObjectiveHudPresenter::Unbind()
{
	if (UWorld* World = BoundWorld.Get())
	{
		if (ULastFPSObjectiveHudSubsystem* Hud = World->GetSubsystem<ULastFPSObjectiveHudSubsystem>())
		{
			Hud->OnPresentationChanged.RemoveDynamic(
				this, &ULastFPSObjectiveHudPresenter::HandlePresentationChanged);
		}
	}
	BoundWorld.Reset();
	ActiveObjective.Reset();
}

void ULastFPSObjectiveHudPresenter::HandlePresentationChanged(
	const ELastFPSObjectiveHudMode Mode,
	UObject* Source)
{
	ActiveObjective = Cast<ULastFPSTimedObjectiveComponent>(Source);
	ApplyMode(Mode);

	ULastFPSTimedObjectiveComponent* Objective = ActiveObjective.Get();
	if (!Objective)
	{
		return;
	}

	// 켜지는 시점에 라벨·전체 시간을 넘겨 준다. 이후 값 갱신은 Tick 이 담당한다.
	if (Mode == ELastFPSObjectiveHudMode::Defend && DefendWidget)
	{
		DefendWidget->SetupObjective(Objective->GetDisplayLabel(), Objective->GetDuration());
		DefendWidget->UpdateDisplay(Objective->GetProgress01(), Objective->GetWatchTargetHealth01());
	}
	else if (Mode == ELastFPSObjectiveHudMode::Capture && CaptureWidget)
	{
		CaptureWidget->SetupObjective(Objective->GetDisplayLabel());
	}
}

void ULastFPSObjectiveHudPresenter::Tick(float /*DeltaTime*/)
{
	ULastFPSTimedObjectiveComponent* Objective = ActiveObjective.Get();
	if (!Objective)
	{
		return;
	}

	const float Progress = Objective->GetProgress01();
	switch (ActiveMode)
	{
	case ELastFPSObjectiveHudMode::Defend:
		if (DefendWidget)
		{
			DefendWidget->UpdateDisplay(Progress, Objective->GetWatchTargetHealth01());
		}
		break;

	case ELastFPSObjectiveHudMode::Capture:
		if (CaptureWidget)
		{
			CaptureWidget->UpdateProgress(Progress);
		}
		break;

	default:
		break;
	}
}

void ULastFPSObjectiveHudPresenter::ApplyMode(const ELastFPSObjectiveHudMode Mode)
{
	ActiveMode = Mode;

	// 슬롯이 하나라 둘 중 최대 하나만 켜진다.
	// 보스 모드에서는 둘 다 꺼지고, 보스 바는 자기 수명에 맞춰 스스로 표시한다.
	SetWidgetVisible(DefendWidget, Mode == ELastFPSObjectiveHudMode::Defend);
	SetWidgetVisible(CaptureWidget, Mode == ELastFPSObjectiveHudMode::Capture);
}

void ULastFPSObjectiveHudPresenter::SetWidgetVisible(UWidget* Widget, const bool bVisible)
{
	if (!Widget)
	{
		return;
	}

	Widget->SetVisibility(bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}
