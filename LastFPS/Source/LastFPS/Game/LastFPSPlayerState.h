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
    /** 궁극기 사용에 필요한 킬 수 (= UltimateGauge 최대값) */
    static constexpr int32 UltimateKillsRequired = 1;
    static constexpr float UltimateKillHealAmount = 100.f;
    static constexpr float UltimateKillHealWindowSeconds = 8.f;

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

    /** 서버: 킬 확정 시 게이지 충전 + 궁극기 킬힐 윈도우 처리 */
    void Auth_OnScoredKill(ALastFPSPlayerState* VictimPS);

    /** 서버: 궁극기 사용 직후 N초간 킬 시 체력 회복 윈도우 시작 */
    void Auth_StartUltimateKillHealWindow(float DurationSeconds);

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
    void Auth_AddUltimateKillCharge(int32 Amount = 1);
    void Auth_TryApplyUltimateKillHeal();

    /** 서버 전용: 이 시각까지 킬 시 궁극기 힐 적용 (World Seconds) */
    float UltimateKillHealWindowEndTime = 0.f;

    bool bGASDefaultsGranted = false;
    UPROPERTY(VisibleAnywhere, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;
};
