#include "Animation/Notifies/AnimNotifyState_LastFPSMeleeTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Utility/LastFPSTags.h"

void UAnimNotifyState_LastFPSMeleeTrace::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SendTraceEvent(MeshComp, LastFPSGameplayTags::Event_Montage_MeleeTrace_Begin, TotalDuration);
}

void UAnimNotifyState_LastFPSMeleeTrace::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	SendTraceEvent(MeshComp, LastFPSGameplayTags::Event_Montage_MeleeTrace_Tick, FrameDeltaTime);
}

void UAnimNotifyState_LastFPSMeleeTrace::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SendTraceEvent(MeshComp, LastFPSGameplayTags::Event_Montage_MeleeTrace_End, 0.f);
}

FString UAnimNotifyState_LastFPSMeleeTrace::GetNotifyName_Implementation() const
{
	return TEXT("Last FPS Melee Trace");
}

void UAnimNotifyState_LastFPSMeleeTrace::SendTraceEvent(
	USkeletalMeshComponent* MeshComp,
	const FGameplayTag EventTag,
	const float EventMagnitude)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !EventTag.IsValid()
		|| !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.Target = Owner;
	Payload.EventMagnitude = EventMagnitude;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}
