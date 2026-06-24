#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_SendGameplayEvent.generated.h"

UCLASS(meta=(DisplayName="Send Gameplay Event"))
class LASTFPS_API UAN_SendGameplayEvent : public UAnimNotify
{
    GENERATED_BODY()

public:
    UAN_SendGameplayEvent();

    virtual FString GetNotifyName_Implementation() const override;
    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay Event", meta=(GameplayTagFilter="GameplayEventTagsCategory"))
    FGameplayTag EventTag;
};
