#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "LastFPSGameStateBase.generated.h"

class ULastFPSDestinationContentComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnLastFPSDestinationContextChanged,
    const FGameplayTagContainer&);

/**
 * 목적지 콘텐츠 게이트의 부착 지점.
 * ULoadingScreenManager 가 GameState 의 컴포넌트를 순회하므로 게이트는 여기 있어야 한다.
 */
UCLASS()
class LASTFPS_API ALastFPSGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALastFPSGameStateBase();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetDestinationContextTags(
        const FGameplayTagContainer& NewContextTags);

    const FGameplayTagContainer& GetDestinationContextTags() const
    {
        return DestinationContextTags;
    }

    FOnLastFPSDestinationContextChanged OnDestinationContextChanged;

private:
    UFUNCTION()
    void OnRep_DestinationContextTags();

    UPROPERTY(VisibleAnywhere, Category="LastFPS|Loading")
    TObjectPtr<ULastFPSDestinationContentComponent> DestinationContent;

    UPROPERTY(ReplicatedUsing=OnRep_DestinationContextTags)
    FGameplayTagContainer DestinationContextTags;
};
