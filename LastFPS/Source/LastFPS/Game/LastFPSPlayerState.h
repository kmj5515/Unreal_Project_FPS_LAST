#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "LastFPSPlayerState.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;

UCLASS()
class LASTFPS_API ALastFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALastFPSPlayerState();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    FORCEINLINE ULastFPSAttributeSet* GetAttributeSet() const { return AttributeSet; }

    bool HasGrantedGASDefaults() const { return bGASDefaultsGranted; }
    void MarkGASDefaultsGranted() { bGASDefaultsGranted = true; }

private:
    bool bGASDefaultsGranted = false;
    UPROPERTY(VisibleAnywhere, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;
};
