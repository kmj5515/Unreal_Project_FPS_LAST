#include "Animation/Notifies/AN_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Utility/LastFPSTags.h"

UAN_SendGameplayEvent::UAN_SendGameplayEvent()
{
#if WITH_EDITORONLY_DATA
    NotifyColor = FColor(80, 170, 255);
    bShouldFireInEditor = false;
#endif
}

FString UAN_SendGameplayEvent::GetNotifyName_Implementation() const
{
    return EventTag.IsValid()
        ? FString::Printf(TEXT("Send Gameplay Event: %s"), *EventTag.ToString())
        : TEXT("Send Gameplay Event");
}

void UAN_SendGameplayEvent::Notify(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("AN_SendGameplayEvent skipped: MeshComp is null"));
        return;
    }

    const UWorld* World = MeshComp->GetWorld();
    if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE))
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("AN_SendGameplayEvent skipped: Owner is null. Animation=%s"), *GetNameSafe(Animation));
        return;
    }

    const FGameplayTag TagToSend = EventTag.IsValid()
        ? EventTag
        : LastFPSGameplayTags::Event_Montage_ProjectileSpawn;
    if (!TagToSend.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("AN_SendGameplayEvent skipped: EventTag is invalid. Owner=%s Animation=%s"),
            *GetNameSafe(Owner),
            *GetNameSafe(Animation));
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = TagToSend;
    Payload.Instigator = Owner;
    Payload.Target = Owner;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, TagToSend, Payload);
}
