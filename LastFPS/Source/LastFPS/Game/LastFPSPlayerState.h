#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "LastFPSPlayerState.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;

/** 개인전(FFA) 매치 통계 — 서버에서만 갱신, 복제로 클라이언트 표시 */
UCLASS()
class LASTFPS_API ALastFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALastFPSPlayerState();

    virtual void PostInitializeComponents() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void CopyProperties(APlayerState* PlayerState) override;
    virtual void OverrideWith(APlayerState* PlayerState) override;

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    FORCEINLINE ULastFPSAttributeSet* GetAttributeSet() const { return AttributeSet; }

    bool HasGrantedGASDefaults() const { return bGASDefaultsGranted; }
    void MarkGASDefaultsGranted() { bGASDefaultsGranted = true; }

    FORCEINLINE int32 GetStatKills() const { return StatKills; }
    FORCEINLINE int32 GetStatDeaths() const { return StatDeaths; }
    FORCEINLINE int32 GetStatAssists() const { return StatAssists; }
    FORCEINLINE float GetStatDamageDealt() const { return StatDamageDealt; }
    FORCEINLINE float GetStatDamageTaken() const { return StatDamageTaken; }
    FORCEINLINE float GetStatHealingReceived() const { return StatHealingReceived; }
    FORCEINLINE float GetStatHealingGiven() const { return StatHealingGiven; }
    FORCEINLINE int32 GetSelectedCharacterIndex() const { return SelectedCharacterIndex; }

    /** GAS AttributeSet 등 서버 전용 — 권한 없으면 무시 */
    void Auth_AddDamageDealt(float Amount);
    void Auth_AddDamageTaken(float Amount);
    void Auth_AddHealingReceived(float Amount);
    void Auth_AddHealingGiven(float Amount);
    void Auth_AddKill();
    void Auth_AddDeath();
    void Auth_AddAssist();
    void Auth_SetSelectedCharacterIndex(int32 NewIndex);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    int32 StatKills = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    int32 StatDeaths = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    int32 StatAssists = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    float StatDamageDealt = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    float StatDamageTaken = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    float StatHealingReceived = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Stats")
    float StatHealingGiven = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="LastFPS|Character")
    int32 SelectedCharacterIndex = 0;

private:
    void Auth_AddFloatStat(float& Stat, float Amount);

    bool bGASDefaultsGranted = false;
    UPROPERTY(VisibleAnywhere, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;
};
