#include "Animation/LastFPSLocomotionAnimationSetTools.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/LastFPSLocomotionAnimationSet.h"

#include <initializer_list>

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#endif

namespace LastFPSLocomotionAnimationSetTools
{
enum class ELocomotionSequenceSlot : uint8
{
	None,
	Idle,
	WalkStart,
	WalkLoop,
	WalkStop,
	JogStart,
	JogLoop,
	JogStop,
	SprintLoop,
	JumpStart,
	JumpStartLoop,
	JumpApex,
	JumpFallLoop,
	JumpFallLand
};

struct FParsedSequenceName
{
	ELocomotionSequenceSlot Slot = ELocomotionSequenceSlot::None;
	EMMCardinalDirection Direction = EMMCardinalDirection::Forward;
	bool bHasPivotMarker = false;
	bool bHasLoopMarker = false;
	int32 NormalizedNameLength = 0;
};

struct FSequenceAssignmentCandidate
{
	FParsedSequenceName Parsed;
	TObjectPtr<UAnimSequenceBase> Sequence;
	int32 Score = MIN_int32;

	bool IsValid() const
	{
		return Sequence != nullptr && Parsed.Slot != ELocomotionSequenceSlot::None;
	}
};

static FString NormalizeContentPath(FString ContentPath)
{
	ContentPath.TrimStartAndEndInline();
	ContentPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	const FString ContentPrefix = TEXT("Content/");
	const FString ProjectContentPrefix = TEXT("LastFPS/Content/");

	if (ContentPath.StartsWith(ProjectContentPrefix))
	{
		ContentPath = TEXT("/Game/") + ContentPath.RightChop(ProjectContentPrefix.Len());
	}
	else if (ContentPath.StartsWith(ContentPrefix))
	{
		ContentPath = TEXT("/Game/") + ContentPath.RightChop(ContentPrefix.Len());
	}

	if (ContentPath.EndsWith(TEXT("/")))
	{
		ContentPath.LeftChopInline(1);
	}

	return ContentPath;
}

static FString NormalizeAssetName(FString AssetName)
{
	AssetName.ToUpperInline();
	AssetName.ReplaceInline(TEXT("-"), TEXT("_"));
	AssetName.ReplaceInline(TEXT(" "), TEXT("_"));
	AssetName.ReplaceInline(TEXT("."), TEXT("_"));
	return AssetName;
}

static void BuildTokens(const FString& NormalizedName, TArray<FString>& OutTokens)
{
	NormalizedName.ParseIntoArray(OutTokens, TEXT("_"), true);
}

static bool HasToken(const TArray<FString>& Tokens, const TCHAR* Candidate)
{
	return Tokens.Contains(FString(Candidate));
}

static bool HasAnyToken(const TArray<FString>& Tokens, std::initializer_list<const TCHAR*> Candidates)
{
	for (const TCHAR* Candidate : Candidates)
	{
		if (HasToken(Tokens, Candidate))
		{
			return true;
		}
	}

	return false;
}

static bool HasAnyText(const FString& Name, std::initializer_list<const TCHAR*> Candidates)
{
	for (const TCHAR* Candidate : Candidates)
	{
		if (Name.Contains(Candidate))
		{
			return true;
		}
	}

	return false;
}

static bool HasStartMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasToken(Tokens, TEXT("START")) || HasAnyText(Name, {TEXT("START"), TEXT("BEGIN")});
}

static bool HasStopMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasToken(Tokens, TEXT("STOP")) || HasAnyText(Name, {TEXT("STOP"), TEXT("END")});
}

static bool HasLoopMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasAnyToken(Tokens, {TEXT("LOOP"), TEXT("CYCLE"), TEXT("PIVOT")})
		|| HasAnyText(Name, {TEXT("LOOP"), TEXT("CYCLE"), TEXT("PIVOT")});
}

static bool HasPivotMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasToken(Tokens, TEXT("PIVOT")) || HasAnyText(Name, {TEXT("PIVOT")});
}

static bool HasExplicitLoopMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasAnyToken(Tokens, {TEXT("LOOP"), TEXT("CYCLE")})
		|| HasAnyText(Name, {TEXT("LOOP"), TEXT("CYCLE")});
}

static bool HasWalkMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasToken(Tokens, TEXT("WALK")) || HasAnyText(Name, {TEXT("WALK")});
}

static bool HasJogMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasAnyToken(Tokens, {TEXT("JOG"), TEXT("RUN")}) || HasAnyText(Name, {TEXT("JOG"), TEXT("RUN")});
}

static bool HasSprintMarker(const FString& Name, const TArray<FString>& Tokens)
{
	return HasToken(Tokens, TEXT("SPRINT")) || HasAnyText(Name, {TEXT("SPRINT")});
}

static EMMCardinalDirection ParseDirection(const FString& Name, const TArray<FString>& Tokens)
{
	if (HasAnyToken(Tokens, {TEXT("RIGHT"), TEXT("R"), TEXT("RT")}) || HasAnyText(Name, {TEXT("RIGHT"), TEXT("_R_"), TEXT("_RT_")}) || Name.EndsWith(TEXT("_R")) || Name.EndsWith(TEXT("_RT")))
	{
		return EMMCardinalDirection::Right;
	}

	if (HasAnyToken(Tokens, {TEXT("BACK"), TEXT("BACKWARD"), TEXT("B"), TEXT("BWD")}) || HasAnyText(Name, {TEXT("BACK"), TEXT("BACKWARD"), TEXT("_B_"), TEXT("_BWD_")}) || Name.EndsWith(TEXT("_B")) || Name.EndsWith(TEXT("_BWD")))
	{
		return EMMCardinalDirection::Back;
	}

	if (HasAnyToken(Tokens, {TEXT("LEFT"), TEXT("L"), TEXT("LT")}) || HasAnyText(Name, {TEXT("LEFT"), TEXT("_L_"), TEXT("_LT_")}) || Name.EndsWith(TEXT("_L")) || Name.EndsWith(TEXT("_LT")))
	{
		return EMMCardinalDirection::Left;
	}

	return EMMCardinalDirection::Forward;
}

static FParsedSequenceName ParseSequenceName(const FString& AssetName)
{
	const FString Name = NormalizeAssetName(AssetName);
	TArray<FString> Tokens;
	BuildTokens(Name, Tokens);

	FParsedSequenceName Parsed;
	Parsed.Direction = ParseDirection(Name, Tokens);
	Parsed.NormalizedNameLength = Name.Len();

	const bool bHasStart = HasStartMarker(Name, Tokens);
	const bool bHasStop = HasStopMarker(Name, Tokens);
	const bool bHasLoop = HasLoopMarker(Name, Tokens);
	Parsed.bHasPivotMarker = HasPivotMarker(Name, Tokens);
	Parsed.bHasLoopMarker = HasExplicitLoopMarker(Name, Tokens);
	const bool bHasJump = HasToken(Tokens, TEXT("JUMP")) || HasAnyText(Name, {TEXT("JUMP")});
	const bool bHasFall = HasToken(Tokens, TEXT("FALL")) || HasAnyText(Name, {TEXT("FALL")});
	const bool bHasLand = HasAnyToken(Tokens, {TEXT("LAND"), TEXT("LANDING")}) || HasAnyText(Name, {TEXT("LAND")});

	if (HasToken(Tokens, TEXT("IDLE")) || HasAnyText(Name, {TEXT("IDLE")}))
	{
		Parsed.Slot = ELocomotionSequenceSlot::Idle;
	}
	else if (bHasJump && bHasStart && bHasLoop)
	{
		Parsed.Slot = ELocomotionSequenceSlot::JumpStartLoop;
	}
	else if (bHasJump && bHasStart)
	{
		Parsed.Slot = ELocomotionSequenceSlot::JumpStart;
	}
	else if (HasToken(Tokens, TEXT("APEX")) || HasAnyText(Name, {TEXT("APEX")}))
	{
		Parsed.Slot = ELocomotionSequenceSlot::JumpApex;
	}
	else if (bHasLand)
	{
		Parsed.Slot = ELocomotionSequenceSlot::JumpFallLand;
	}
	else if (bHasFall)
	{
		Parsed.Slot = ELocomotionSequenceSlot::JumpFallLoop;
	}
	else if (HasWalkMarker(Name, Tokens))
	{
		Parsed.Slot = bHasStart
			? ELocomotionSequenceSlot::WalkStart
			: bHasStop
				? ELocomotionSequenceSlot::WalkStop
				: ELocomotionSequenceSlot::WalkLoop;
	}
	else if (HasSprintMarker(Name, Tokens))
	{
		Parsed.Slot = ELocomotionSequenceSlot::SprintLoop;
	}
	else if (HasJogMarker(Name, Tokens))
	{
		Parsed.Slot = bHasStart
			? ELocomotionSequenceSlot::JogStart
			: bHasStop
				? ELocomotionSequenceSlot::JogStop
				: ELocomotionSequenceSlot::JogLoop;
	}

	return Parsed;
}

static int32 GetDirectionIndex(EMMCardinalDirection Direction)
{
	switch (Direction)
	{
	case EMMCardinalDirection::Right:
		return 1;
	case EMMCardinalDirection::Back:
		return 2;
	case EMMCardinalDirection::Left:
		return 3;
	case EMMCardinalDirection::Forward:
	default:
		return 0;
	}
}

static int32 GetCandidateIndex(const FParsedSequenceName& Parsed)
{
	const int32 SlotIndex = static_cast<int32>(Parsed.Slot);
	const bool bDirectionalSlot =
		Parsed.Slot == ELocomotionSequenceSlot::WalkStart
		|| Parsed.Slot == ELocomotionSequenceSlot::WalkLoop
		|| Parsed.Slot == ELocomotionSequenceSlot::WalkStop
		|| Parsed.Slot == ELocomotionSequenceSlot::JogStart
		|| Parsed.Slot == ELocomotionSequenceSlot::JogLoop
		|| Parsed.Slot == ELocomotionSequenceSlot::JogStop;

	return SlotIndex * 4 + (bDirectionalSlot ? GetDirectionIndex(Parsed.Direction) : 0);
}

static int32 GetSequenceScore(const FParsedSequenceName& Parsed)
{
	int32 Score = 1000;

	if (Parsed.Slot == ELocomotionSequenceSlot::WalkLoop || Parsed.Slot == ELocomotionSequenceSlot::JogLoop)
	{
		Score += Parsed.bHasPivotMarker ? -100 : 100;
		Score += Parsed.bHasLoopMarker ? 10 : 20;
	}

	Score -= Parsed.NormalizedNameLength;
	return Score;
}

static bool IsBetterCandidate(const FSequenceAssignmentCandidate& CurrentCandidate, const FParsedSequenceName& Parsed)
{
	const int32 NewScore = GetSequenceScore(Parsed);
	if (!CurrentCandidate.IsValid())
	{
		return true;
	}

	return NewScore > CurrentCandidate.Score;
}

static TObjectPtr<UAnimSequenceBase>& SelectDirectionalSequence(
	FLastFPSDirectionalSequenceSet& SequenceSet,
	EMMCardinalDirection Direction)
{
	switch (Direction)
	{
	case EMMCardinalDirection::Right:
		return SequenceSet.Right;
	case EMMCardinalDirection::Back:
		return SequenceSet.Back;
	case EMMCardinalDirection::Left:
		return SequenceSet.Left;
	case EMMCardinalDirection::Forward:
	default:
		return SequenceSet.Forward;
	}
}

static bool AssignSequence(
	TObjectPtr<UAnimSequenceBase>& Target,
	UAnimSequenceBase* Sequence,
	bool bOverwriteExisting)
{
	if (!Sequence || (Target && !bOverwriteExisting))
	{
		return false;
	}

	if (Target == Sequence)
	{
		return false;
	}

	Target = Sequence;
	return true;
}

static bool AssignParsedSequence(
	FLastFPSHeroLinkedLocomotionSequences& LocomotionSequences,
	const FParsedSequenceName& Parsed,
	UAnimSequenceBase* Sequence,
	bool bOverwriteExisting)
{
	switch (Parsed.Slot)
	{
	case ELocomotionSequenceSlot::Idle:
		return AssignSequence(LocomotionSequences.Idle, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::WalkStart:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.WalkStart, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::WalkLoop:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.WalkLoop, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::WalkStop:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.WalkStop, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JogStart:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.JogStart, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JogLoop:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.JogLoop, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JogStop:
		return AssignSequence(SelectDirectionalSequence(LocomotionSequences.JogStop, Parsed.Direction), Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::SprintLoop:
		return AssignSequence(LocomotionSequences.SprintLoop, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JumpStart:
		return AssignSequence(LocomotionSequences.JumpStart, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JumpStartLoop:
		return AssignSequence(LocomotionSequences.JumpStartLoop, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JumpApex:
		return AssignSequence(LocomotionSequences.JumpApex, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JumpFallLoop:
		return AssignSequence(LocomotionSequences.JumpFallLoop, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::JumpFallLand:
		return AssignSequence(LocomotionSequences.JumpFallLand, Sequence, bOverwriteExisting);
	case ELocomotionSequenceSlot::None:
	default:
		return false;
	}
}
}

int32 ULastFPSLocomotionAnimationSetTools::AutoFillLocomotionAnimationSet(
	ULastFPSLocomotionAnimationSet* AnimationSet,
	const FString& ContentPath,
	bool bOverwriteExisting,
	bool bClearBeforeFill)
{
	return AutoFillLocomotionAnimationSetWithNameFilter(
		AnimationSet,
		ContentPath,
		FString(),
		bOverwriteExisting,
		bClearBeforeFill);
}

int32 ULastFPSLocomotionAnimationSetTools::AutoFillLocomotionAnimationSetWithNameFilter(
	ULastFPSLocomotionAnimationSet* AnimationSet,
	const FString& ContentPath,
	const FString& RequiredNameText,
	bool bOverwriteExisting,
	bool bClearBeforeFill)
{
	return AutoFillLocomotionAnimationSetWithFilters(
		AnimationSet,
		ContentPath,
		RequiredNameText,
		FString(),
		bOverwriteExisting,
		bClearBeforeFill);
}

int32 ULastFPSLocomotionAnimationSetTools::AutoFillLocomotionAnimationSetWithFilters(
	ULastFPSLocomotionAnimationSet* AnimationSet,
	const FString& ContentPath,
	const FString& RequiredNameText,
	const FString& RequiredPrefixText,
	bool bOverwriteExisting,
	bool bClearBeforeFill)
{
#if WITH_EDITOR
	if (!AnimationSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("AutoFillLocomotionAnimationSet failed: AnimationSet is null."));
		return 0;
	}

	const FString PackagePath = LastFPSLocomotionAnimationSetTools::NormalizeContentPath(ContentPath);
	if (!PackagePath.StartsWith(TEXT("/Game")))
	{
		UE_LOG(LogTemp, Warning, TEXT("AutoFillLocomotionAnimationSet failed: ContentPath must be a /Game path. Path=%s"), *PackagePath);
		return 0;
	}

	AnimationSet->Modify();
	if (bClearBeforeFill)
	{
		AnimationSet->LocomotionSequences = FLastFPSHeroLinkedLocomotionSequences();
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.PackagePaths.Add(*PackagePath);
	Filter.ClassPaths.Add(UAnimSequenceBase::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	const FString NormalizedRequiredNameText =
		LastFPSLocomotionAnimationSetTools::NormalizeAssetName(RequiredNameText);
	const FString NormalizedRequiredPrefixText =
		LastFPSLocomotionAnimationSetTools::NormalizeAssetName(RequiredPrefixText);

	int32 AssignedCount = 0;
	TArray<LastFPSLocomotionAnimationSetTools::FSequenceAssignmentCandidate> Candidates;
	Candidates.SetNum(static_cast<int32>(LastFPSLocomotionAnimationSetTools::ELocomotionSequenceSlot::JumpFallLand) * 4 + 4);

	for (const FAssetData& AssetData : AssetDataList)
	{
		const FString NormalizedAssetName =
			LastFPSLocomotionAnimationSetTools::NormalizeAssetName(AssetData.AssetName.ToString());
		if (!NormalizedRequiredPrefixText.IsEmpty() && !NormalizedAssetName.StartsWith(NormalizedRequiredPrefixText))
		{
			continue;
		}

		const FString NormalizedAssetIdentity =
			LastFPSLocomotionAnimationSetTools::NormalizeAssetName(
				AssetData.PackageName.ToString() + TEXT("_") + AssetData.AssetName.ToString());
		if (!NormalizedRequiredNameText.IsEmpty() && !NormalizedAssetIdentity.Contains(NormalizedRequiredNameText))
		{
			continue;
		}

		UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(AssetData.GetAsset());
		if (!Sequence)
		{
			continue;
		}

		const LastFPSLocomotionAnimationSetTools::FParsedSequenceName Parsed =
			LastFPSLocomotionAnimationSetTools::ParseSequenceName(AssetData.AssetName.ToString());
		if (Parsed.Slot == LastFPSLocomotionAnimationSetTools::ELocomotionSequenceSlot::None)
		{
			continue;
		}

		const int32 CandidateIndex = LastFPSLocomotionAnimationSetTools::GetCandidateIndex(Parsed);
		if (!Candidates.IsValidIndex(CandidateIndex))
		{
			continue;
		}

		if (LastFPSLocomotionAnimationSetTools::IsBetterCandidate(Candidates[CandidateIndex], Parsed))
		{
			Candidates[CandidateIndex].Parsed = Parsed;
			Candidates[CandidateIndex].Sequence = Sequence;
			Candidates[CandidateIndex].Score = LastFPSLocomotionAnimationSetTools::GetSequenceScore(Parsed);
		}
	}

	for (const LastFPSLocomotionAnimationSetTools::FSequenceAssignmentCandidate& Candidate : Candidates)
	{
		if (!Candidate.IsValid())
		{
			continue;
		}

		if (LastFPSLocomotionAnimationSetTools::AssignParsedSequence(
			AnimationSet->LocomotionSequences,
			Candidate.Parsed,
			Candidate.Sequence,
			bOverwriteExisting))
		{
			++AssignedCount;
		}
	}

	AnimationSet->MarkPackageDirty();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("AutoFillLocomotionAnimationSet completed: %d sequences assigned from %s. NameFilter=%s PrefixFilter=%s"),
		AssignedCount,
		*PackagePath,
		NormalizedRequiredNameText.IsEmpty() ? TEXT("None") : *NormalizedRequiredNameText,
		NormalizedRequiredPrefixText.IsEmpty() ? TEXT("None") : *NormalizedRequiredPrefixText);
	return AssignedCount;
#else
	return 0;
#endif
}
