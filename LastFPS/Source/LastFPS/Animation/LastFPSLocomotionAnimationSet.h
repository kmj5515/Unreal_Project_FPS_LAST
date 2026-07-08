#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LastFPSLocomotionAnimationSet.generated.h"

class UAnimSequenceBase;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSDirectionalSequenceSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Forward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Left;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSLeftRightSequenceSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Right;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSTurnInPlaceSequenceSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSLeftRightSequenceSet Turn90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSLeftRightSequenceSet Turn180;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSHeroLinkedLocomotionSequences
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkStop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogStop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet Pivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSTurnInPlaceSequenceSet TurnInPlace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> SprintLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpStartLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpApex;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpFallLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpFallLand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpAdditiveRecovery;
};

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSLocomotionAnimationSet : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> Idle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet WalkStop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet JogStop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSDirectionalSequenceSet Pivot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	FLastFPSTurnInPlaceSequenceSet TurnInPlace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> SprintLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpStartLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpApex;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpFallLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpFallLand;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MM|Animation")
	TObjectPtr<UAnimSequenceBase> JumpAdditiveRecovery;

	UPROPERTY(BlueprintReadOnly, Category="MM|Animation")
	FLastFPSHeroLinkedLocomotionSequences LocomotionSequences;

	const FLastFPSHeroLinkedLocomotionSequences& GetLocomotionSequences() const;
	void ClearSequences();
	void SyncLocomotionSequencesFromSeparatedSequences();

private:
	UPROPERTY()
	bool bUseSeparatedSequenceStorage = false;

	mutable FLastFPSHeroLinkedLocomotionSequences CachedLocomotionSequences;

	bool ShouldUseSeparatedSequences() const;
	bool HasSeparatedSequences() const;
	bool HasLegacyLocomotionSequences() const;
	void MigrateLegacyLocomotionSequences();
	void RebuildCachedLocomotionSequences() const;
};
