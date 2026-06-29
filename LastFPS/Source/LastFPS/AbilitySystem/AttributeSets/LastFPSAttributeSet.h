#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LastFPSAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)           \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class LASTFPS_API ULastFPSAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    ULastFPSAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // Health
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Attributes|Vitals")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Attributes|Vitals")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, MaxHealth)

    // Stamina
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stamina, Category="Attributes|Vitals")
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, Stamina)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxStamina, Category="Attributes|Vitals")
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, MaxStamina)

    // Combat
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackDamage, Category="Attributes|Combat")
    FGameplayAttributeData AttackDamage;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, AttackDamage)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CriticalChance, Category="Attributes|Combat")
    FGameplayAttributeData CriticalChance;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, CriticalChance)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CriticalDamagePercent, Category="Attributes|Combat")
    FGameplayAttributeData CriticalDamagePercent;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, CriticalDamagePercent)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Defense, Category="Attributes|Combat")
    FGameplayAttributeData Defense;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, Defense)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, Category="Attributes|Movement")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, MoveSpeed)

    // Meta attribute — 피해 계산용 임시값, 복제 안 함
    UPROPERTY(BlueprintReadOnly, Category="Attributes|Meta")
    FGameplayAttributeData Damage;
    ATTRIBUTE_ACCESSORS(ULastFPSAttributeSet, Damage)

protected:
    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Stamina(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxStamina(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_AttackDamage(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_CriticalChance(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_CriticalDamagePercent(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Defense(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& Old);

private:
    void HandleDamageEffect(const FGameplayEffectModCallbackData& Data);
    void HandleHealEffect(const FGameplayEffectModCallbackData& Data);
};
