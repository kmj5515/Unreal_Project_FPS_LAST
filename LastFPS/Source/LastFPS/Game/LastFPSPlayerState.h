#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/Tables/LastFPSEquipmentStatTypes.h"
#include "LastFPSPlayerState.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FLastFPSDamageDealtSignature,
    float, DamageAmount,
    float, TotalDamageDealt,
    FVector, DamageWorldLocation,
    AActor*, DamageTargetActor,
    bool, bCriticalHit);

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

    /** 허브/맵 이동 시 부여되는 현재 레벨의 제한(전투 금지 등) 효과 핸들. GameMode가 관리한다. */
    FActiveGameplayEffectHandle LevelRestrictionHandle;

    UPROPERTY(BlueprintAssignable, Category="LastFPS|Damage")
    FLastFPSDamageDealtSignature OnDamageDealt;

    /** GAS AttributeSet 등 서버 전용 — 권한 없으면 무시 */
    void Auth_AddDamageDealt(
        float Amount,
        const FVector& DamageWorldLocation = FVector::ZeroVector,
        AActor* DamageTargetActor = nullptr,
        bool bCriticalHit = false);
    void Auth_AddDamageTaken(float Amount);
    void Auth_AddHealingReceived(float Amount);
    void Auth_AddHealingGiven(float Amount);
    void Auth_AddKill();
    void Auth_AddDeath();
    void Auth_AddAssist();
    void Auth_SetSelectedCharacterIndex(int32 NewIndex);

    // 서버: 이 플레이어 소유 클라의 로컬 Economy 에 인게임 드랍 아이템 지급.
    void Auth_GrantItem(FName ItemRowId, int32 Count);

    /**
     * 서버: 소유 클라이언트가 제출한 장비 구성을 확정한다.
     * 형식 검증을 통과한 항목만 남기고, 이미 폰이 있으면 즉시 다시 반영한다.
     */
    void Auth_SetEquippedSlots(const TArray<FLastFPSEquippedSlot>& NewSlots);

    /** 이 플레이어의 장비 구성. 서버가 폰 스탯·무기 로드아웃을 만들 때의 단일 출처다. */
    const TArray<FLastFPSEquippedSlot>& GetEquippedSlots() const { return EquippedSlots; }

    /**
     * 서버: 처치 목표를 이 플레이어의 로컬 퀘스트 진행에 통지한다.
     * 퀘스트 진행은 GameInstance 소유라 서버에서 직접 올리면 리슨 서버 호스트에게만 쌓인다.
     */
    void Auth_NotifyObjectiveKill(FGameplayTag KillTag);

    /** 장비 보정 GE 핸들. 구성이 바뀌면 이전 것을 제거하고 다시 적용해야 중첩되지 않는다. */
    FActiveGameplayEffectHandle EquipmentStatsHandle;

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

    /**
     * 이 플레이어가 장착한 아이템 행 목록. 서버가 확정하고 전원에게 복제한다.
     * 다른 클라이언트도 관전·스코어보드에서 상대 장비를 읽을 수 있어야 하므로 소유자 한정으로 두지 않는다.
     */
    UPROPERTY(Replicated)
    TArray<FLastFPSEquippedSlot> EquippedSlots;

private:
    void Auth_AddFloatStat(float& Stat, float Amount);

    UFUNCTION(Client, Unreliable)
    void Client_NotifyDamageDealt(
        float DamageAmount,
        float TotalDamageDealt,
        FVector DamageWorldLocation,
        AActor* DamageTargetActor,
        bool bCriticalHit);

    // 아이템/화폐 상태라 Reliable (데미지 알림과 달리).
    UFUNCTION(Client, Reliable)
    void Client_GrantItem(FName ItemRowId, int32 Count);

    // 퀘스트 진행도 누락되면 복구되지 않으므로 Reliable.
    UFUNCTION(Client, Reliable)
    void Client_NotifyObjectiveKill(FGameplayTag KillTag);

    void NotifyObjectiveKillLocal(FGameplayTag KillTag);

    void GrantItemLocal(FName ItemRowId, int32 Count);

    bool bGASDefaultsGranted = false;
    UPROPERTY(VisibleAnywhere, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;
};
