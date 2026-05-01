#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "LastFPSCharacterBase.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

UCLASS(Abstract)
class LASTFPS_API ALastFPSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALastFPSCharacterBase();

    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    bool IsAlive() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetHealth() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetMaxHealth() const;

    virtual bool GetIsADS() const { return false; }

protected:
    virtual void BeginPlay() override;

    // 서버: Pawn 빙의 시 ASC 초기화
    virtual void PossessedBy(AController* NewController) override;
    // 클라이언트: PlayerState 복제 완료 시 ASC 초기화
    virtual void OnRep_PlayerState() override;

    void InitAbilitySystem();
    void GiveDefaultAbilities();
    void ApplyDefaultEffects();
    void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

    // Phase 1에서는 ASC를 캐릭터에 직접 보유
    // Phase 3에서 PlayerState로 이전 예정
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;

    // 기본 어빌리티 목록 (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스폰 시 즉시 적용할 기본 Effect (초기 스탯 세팅용)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;
};
