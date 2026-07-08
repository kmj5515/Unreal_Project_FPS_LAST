#include "Animation/LastFPSLocomotionAnimationSet.h"

#include "Animation/AnimSequenceBase.h"

namespace LastFPSLocomotionAnimationSet
{
static bool HasSequence(const TObjectPtr<UAnimSequenceBase>& Sequence)
{
	return Sequence != nullptr;
}

static bool HasDirectionalSequences(const FLastFPSDirectionalSequenceSet& SequenceSet)
{
	return HasSequence(SequenceSet.Forward)
		|| HasSequence(SequenceSet.Right)
		|| HasSequence(SequenceSet.Back)
		|| HasSequence(SequenceSet.Left);
}

static bool HasLeftRightSequences(const FLastFPSLeftRightSequenceSet& SequenceSet)
{
	return HasSequence(SequenceSet.Left) || HasSequence(SequenceSet.Right);
}

static bool HasTurnInPlaceSequences(const FLastFPSTurnInPlaceSequenceSet& SequenceSet)
{
	return HasLeftRightSequences(SequenceSet.Turn90) || HasLeftRightSequences(SequenceSet.Turn180);
}

static void CopyDirectionalSequences(
	FLastFPSDirectionalSequenceSet& Target,
	const FLastFPSDirectionalSequenceSet& Source)
{
	Target.Forward = Source.Forward;
	Target.Right = Source.Right;
	Target.Back = Source.Back;
	Target.Left = Source.Left;
}

static void CopyLeftRightSequences(
	FLastFPSLeftRightSequenceSet& Target,
	const FLastFPSLeftRightSequenceSet& Source)
{
	Target.Left = Source.Left;
	Target.Right = Source.Right;
}

static void CopyTurnInPlaceSequences(
	FLastFPSTurnInPlaceSequenceSet& Target,
	const FLastFPSTurnInPlaceSequenceSet& Source)
{
	CopyLeftRightSequences(Target.Turn90, Source.Turn90);
	CopyLeftRightSequences(Target.Turn180, Source.Turn180);
}
}

void ULastFPSLocomotionAnimationSet::PostLoad()
{
	Super::PostLoad();
	MigrateLegacyLocomotionSequences();
	SyncLocomotionSequencesFromSeparatedSequences();
}

#if WITH_EDITOR
void ULastFPSLocomotionAnimationSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncLocomotionSequencesFromSeparatedSequences();
}
#endif

const FLastFPSHeroLinkedLocomotionSequences& ULastFPSLocomotionAnimationSet::GetLocomotionSequences() const
{
	if (!ShouldUseSeparatedSequences())
	{
		return LocomotionSequences;
	}

	RebuildCachedLocomotionSequences();
	return CachedLocomotionSequences;
}

void ULastFPSLocomotionAnimationSet::ClearSequences()
{
	bUseSeparatedSequenceStorage = true;

	Idle = nullptr;
	WalkStart = FLastFPSDirectionalSequenceSet();
	WalkLoop = FLastFPSDirectionalSequenceSet();
	WalkStop = FLastFPSDirectionalSequenceSet();
	JogStart = FLastFPSDirectionalSequenceSet();
	JogLoop = FLastFPSDirectionalSequenceSet();
	JogStop = FLastFPSDirectionalSequenceSet();
	Pivot = FLastFPSDirectionalSequenceSet();
	TurnInPlace = FLastFPSTurnInPlaceSequenceSet();
	SprintLoop = nullptr;
	JumpStart = nullptr;
	JumpStartLoop = nullptr;
	JumpApex = nullptr;
	JumpFallLoop = nullptr;
	JumpFallLand = nullptr;
	JumpAdditiveRecovery = nullptr;
	LocomotionSequences = FLastFPSHeroLinkedLocomotionSequences();
	CachedLocomotionSequences = FLastFPSHeroLinkedLocomotionSequences();
}

void ULastFPSLocomotionAnimationSet::SyncLocomotionSequencesFromSeparatedSequences()
{
	if (!ShouldUseSeparatedSequences())
	{
		return;
	}

	LocomotionSequences.Idle = Idle;
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.WalkStart, WalkStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.WalkLoop, WalkLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.WalkStop, WalkStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.JogStart, JogStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.JogLoop, JogLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.JogStop, JogStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(LocomotionSequences.Pivot, Pivot);
	LastFPSLocomotionAnimationSet::CopyTurnInPlaceSequences(LocomotionSequences.TurnInPlace, TurnInPlace);
	LocomotionSequences.SprintLoop = SprintLoop;
	LocomotionSequences.JumpStart = JumpStart;
	LocomotionSequences.JumpStartLoop = JumpStartLoop;
	LocomotionSequences.JumpApex = JumpApex;
	LocomotionSequences.JumpFallLoop = JumpFallLoop;
	LocomotionSequences.JumpFallLand = JumpFallLand;
	LocomotionSequences.JumpAdditiveRecovery = JumpAdditiveRecovery;
}

bool ULastFPSLocomotionAnimationSet::ShouldUseSeparatedSequences() const
{
	return bUseSeparatedSequenceStorage || HasSeparatedSequences();
}

bool ULastFPSLocomotionAnimationSet::HasSeparatedSequences() const
{
	return LastFPSLocomotionAnimationSet::HasSequence(Idle)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(WalkStart)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(WalkLoop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(WalkStop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(JogStart)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(JogLoop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(JogStop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(Pivot)
		|| LastFPSLocomotionAnimationSet::HasTurnInPlaceSequences(TurnInPlace)
		|| LastFPSLocomotionAnimationSet::HasSequence(SprintLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpStart)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpStartLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpApex)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpFallLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpFallLand)
		|| LastFPSLocomotionAnimationSet::HasSequence(JumpAdditiveRecovery);
}

bool ULastFPSLocomotionAnimationSet::HasLegacyLocomotionSequences() const
{
	return LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.Idle)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.WalkStart)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.WalkLoop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.WalkStop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.JogStart)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.JogLoop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.JogStop)
		|| LastFPSLocomotionAnimationSet::HasDirectionalSequences(LocomotionSequences.Pivot)
		|| LastFPSLocomotionAnimationSet::HasTurnInPlaceSequences(LocomotionSequences.TurnInPlace)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.SprintLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpStart)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpStartLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpApex)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpFallLoop)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpFallLand)
		|| LastFPSLocomotionAnimationSet::HasSequence(LocomotionSequences.JumpAdditiveRecovery);
}

void ULastFPSLocomotionAnimationSet::MigrateLegacyLocomotionSequences()
{
	if (bUseSeparatedSequenceStorage || HasSeparatedSequences() || !HasLegacyLocomotionSequences())
	{
		bUseSeparatedSequenceStorage = bUseSeparatedSequenceStorage || HasSeparatedSequences();
		return;
	}

	Idle = LocomotionSequences.Idle;
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(WalkStart, LocomotionSequences.WalkStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(WalkLoop, LocomotionSequences.WalkLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(WalkStop, LocomotionSequences.WalkStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(JogStart, LocomotionSequences.JogStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(JogLoop, LocomotionSequences.JogLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(JogStop, LocomotionSequences.JogStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(Pivot, LocomotionSequences.Pivot);
	LastFPSLocomotionAnimationSet::CopyTurnInPlaceSequences(TurnInPlace, LocomotionSequences.TurnInPlace);
	SprintLoop = LocomotionSequences.SprintLoop;
	JumpStart = LocomotionSequences.JumpStart;
	JumpStartLoop = LocomotionSequences.JumpStartLoop;
	JumpApex = LocomotionSequences.JumpApex;
	JumpFallLoop = LocomotionSequences.JumpFallLoop;
	JumpFallLand = LocomotionSequences.JumpFallLand;
	JumpAdditiveRecovery = LocomotionSequences.JumpAdditiveRecovery;
	bUseSeparatedSequenceStorage = true;
}

void ULastFPSLocomotionAnimationSet::RebuildCachedLocomotionSequences() const
{
	CachedLocomotionSequences.Idle = Idle;
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.WalkStart, WalkStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.WalkLoop, WalkLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.WalkStop, WalkStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.JogStart, JogStart);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.JogLoop, JogLoop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.JogStop, JogStop);
	LastFPSLocomotionAnimationSet::CopyDirectionalSequences(CachedLocomotionSequences.Pivot, Pivot);
	LastFPSLocomotionAnimationSet::CopyTurnInPlaceSequences(CachedLocomotionSequences.TurnInPlace, TurnInPlace);
	CachedLocomotionSequences.SprintLoop = SprintLoop;
	CachedLocomotionSequences.JumpStart = JumpStart;
	CachedLocomotionSequences.JumpStartLoop = JumpStartLoop;
	CachedLocomotionSequences.JumpApex = JumpApex;
	CachedLocomotionSequences.JumpFallLoop = JumpFallLoop;
	CachedLocomotionSequences.JumpFallLand = JumpFallLand;
	CachedLocomotionSequences.JumpAdditiveRecovery = JumpAdditiveRecovery;
}
