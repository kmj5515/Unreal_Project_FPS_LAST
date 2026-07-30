#include "UI/HUD/LastFPSObjectiveHudSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSObjectiveHud, Log, All);

bool ULastFPSObjectiveHudSubsystem::RequestPresentation(
	const ELastFPSObjectiveHudMode Mode,
	UObject* Source)
{
	if (!Source || Mode == ELastFPSObjectiveHudMode::None)
	{
		return false;
	}

	PruneDeadSource();

	UObject* Current = ActiveSource.Get();
	if (Current && Current != Source)
	{
		// 설계상 동시 표시는 없다. 조용히 덮어쓰면 어느 표시가 사라졌는지 알 수 없으므로 남긴다.
		UE_LOG(
			LogLastFPSObjectiveHud,
			Warning,
			TEXT("[HUD] 목표 표시가 이미 점유돼 요청을 거부했습니다: 점유=%s(%d), 요청=%s(%d)"),
			*GetNameSafe(Current),
			static_cast<int32>(ActiveMode),
			*GetNameSafe(Source),
			static_cast<int32>(Mode));
		return false;
	}

	if (Current == Source && ActiveMode == Mode)
	{
		return true;
	}

	ActiveMode = Mode;
	ActiveSource = Source;
	Broadcast();
	return true;
}

void ULastFPSObjectiveHudSubsystem::ReleasePresentation(UObject* Source)
{
	// 늦게 도착한 해제가 다음 목표의 표시를 꺼버리지 않도록 점유자 본인만 반납할 수 있다.
	if (!Source || ActiveSource.Get() != Source)
	{
		PruneDeadSource();
		return;
	}

	ActiveMode = ELastFPSObjectiveHudMode::None;
	ActiveSource.Reset();
	Broadcast();
}

void ULastFPSObjectiveHudSubsystem::Deinitialize()
{
	ActiveMode = ELastFPSObjectiveHudMode::None;
	ActiveSource.Reset();
	OnPresentationChanged.Clear();
	Super::Deinitialize();
}

bool ULastFPSObjectiveHudSubsystem::PruneDeadSource()
{
	if (ActiveMode == ELastFPSObjectiveHudMode::None || ActiveSource.IsValid())
	{
		return false;
	}

	// 반납 없이 사라진 주체 — 슬롯이 영구 점유되면 다음 목표가 표시되지 못한다.
	ActiveMode = ELastFPSObjectiveHudMode::None;
	ActiveSource.Reset();
	Broadcast();
	return true;
}

void ULastFPSObjectiveHudSubsystem::Broadcast()
{
	OnPresentationChanged.Broadcast(ActiveMode, ActiveSource.Get());
}
