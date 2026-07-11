#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "Data/Status/LastFPSStatusOverlayConfig.h"
#include "LastFPSCharacterBase.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class ULastFPSCharacterDefinition;
class UGameplayEffect;
class UGameplayAbility;
class ULastFPSStatusOverlayConfig;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class APlayerState;

class ALastFPSCharacterBase;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLastFPSCharacterDeath, ALastFPSCharacterBase* /*DeadChar*/);

UCLASS(Abstract)
class LASTFPS_API ALastFPSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALastFPSCharacterBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // IAbilitySystemInterface — PlayerState의 ASC를 반환
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    bool IsAlive() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetHealth() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetMaxHealth() const;

    /** AttributeSet 의 공격 사거리(cm). AI 추격/공격 판정이 사용. 0 이면 미설정. */
    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetAttackRange() const;

    /** 킬피드 등 UI 표시명. 비어 있으면 PlayerState 이름 사용 */
    UFUNCTION(BlueprintPure, Category="LastFPS|Display")
    FString GetKillFeedDisplayName() const;

    /** Pawn 없을 때 PlayerState만으로 표시명 해석 */
    static FString GetKillFeedDisplayNameForPlayerState(const APlayerState* PS);

    virtual bool GetIsADS() const { return false; }

    UFUNCTION(BlueprintPure, Category="LastFPS|Combat")
    bool IsInCombat() const { return bIsInCombat; }

    UFUNCTION(BlueprintCallable, Category="LastFPS|Combat")
    void MarkCombatEngaged();

    UFUNCTION(BlueprintPure, Category="LastFPS|Character")
    const ULastFPSCharacterDefinition* GetCharacterDefinition() const;

    void SetCharacterDefinitionForSpawn(ULastFPSCharacterDefinition* InDefinition);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayHitSound();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SetStatusOverlayMaterial(
        UMaterialInterface* OverlayMaterial,
        FName MixParameterName,
        float MixValue,
        bool bInterpolateMix,
        float MixInterpSpeed);

    /** 서버: 명중 처리 후 발사자 클라이언트에서만 히트마커 표시 */
    UFUNCTION(Client, Reliable)
    void Client_NotifyHitMarker();

    // ── 어시스트 추적 (서버 전용) ──────────────────────────────
    static constexpr float AssistTimeWindow = 10.f;

    void RecordAttacker(APlayerState* Attacker);
    void ClearRecentAttackers();
    const TMap<TWeakObjectPtr<APlayerState>, float>& GetRecentAttackers() const { return RecentAttackers; }

    // 서버: HP 0 도달 시 1회 호출되는 사망 훅. 래치되어 중복 호출은 무시된다.
    void HandleDeath();

    // 서버 사망 시 브로드캐스트. 드랍/미션 등이 구독한다.
    FOnLastFPSCharacterDeath OnDeath;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 서버: Pawn 빙의 시 ASC 초기화
    virtual void PossessedBy(AController* NewController) override;
    // 클라이언트: PlayerState 복제 완료 시 ASC 초기화
    virtual void OnRep_PlayerState() override;
    virtual void PostInitializeComponents() override;

    void InitAbilitySystem();
    virtual void GiveDefaultAbilities();
    void ApplyDefaultEffects();
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void UpdateAliveCollisionState(bool bAlive);
    void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
    void BindStatusOverlayMaterials(UAbilitySystemComponent* ASC);
    void UnbindStatusOverlayMaterials(UAbilitySystemComponent* ASC);
    void OnStatusOverlayTagChanged(FGameplayTag StatusTag, int32 NewCount);
    void RefreshStatusOverlayMaterial();
    bool IsStatusOverlayActive(UAbilitySystemComponent* ASC, const FLastFPSStatusOverlayMaterial& OverlayConfig) const;
    float GetStatusOverlayMix(UAbilitySystemComponent* ASC, const FLastFPSStatusOverlayMaterial& OverlayConfig) const;
    int32 GetStatusOverlayStackCount(UAbilitySystemComponent* ASC, const FLastFPSStatusOverlayMaterial& OverlayConfig) const;
    int32 GetStatusOverlayFullStackCount(const FLastFPSStatusOverlayMaterial& OverlayConfig) const;
    void ApplyStatusOverlayMaterial(
        UMaterialInterface* OverlayMaterial,
        FName MixParameterName,
        float MixValue,
        bool bInterpolateMix,
        float MixInterpSpeed);
    void UpdateStatusOverlayMixInterpolation();
    const ULastFPSCharacterDefinition* ResolveCharacterDefinition() const;
    void ApplyCharacterVisuals(const ULastFPSCharacterDefinition* Definition);
    void ClearCombatEngaged();
    virtual void OnCombatEngagedChanged();

    // Character-owned GAS used when there is no PlayerState ASC.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> OwnedAbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> OwnedAttributeSet;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;

    bool bOwnedGASDefaultsGranted = false;
    bool bHasDied = false;
    TWeakObjectPtr<UAbilitySystemComponent> BoundAttributeASC;

    /** BP/에디터에서 캐릭터별 닉네임 지정. 비어 있으면 GetPlayerName() */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Display")
    FString CharacterNickname;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Character")
    TObjectPtr<ULastFPSCharacterDefinition> CharacterDefinition;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Sound")
    TObjectPtr<USoundBase> HitSound;

    // 기본 어빌리티 목록 (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스폰 시 즉시 적용할 기본 Effect (초기 스탯 세팅용)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
    TObjectPtr<ULastFPSStatusOverlayConfig> StatusOverlayConfig;

    FDelegateHandle MoveSpeedDelegateHandle;
    FDelegateHandle HealthDelegateHandle;
    TMap<FGameplayTag, FDelegateHandle> StatusOverlayDelegateHandles;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> ActiveStatusOverlayMID;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> ActiveStatusOverlaySourceMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Combat", meta=(ClampMin="0.0"))
    float CombatEngagedDuration = 3.f;

    UPROPERTY(ReplicatedUsing=OnRep_IsInCombat, BlueprintReadOnly, Category="LastFPS|Combat")
    bool bIsInCombat = false;

    FTimerHandle StatusOverlayMixInterpolationTimerHandle;
    FTimerHandle CombatEngagedTimerHandle;
    FName ActiveStatusOverlayMixParameterName = NAME_None;
    float ActiveStatusOverlayMixValue = 0.f;
    float TargetStatusOverlayMixValue = 0.f;
    float ActiveStatusOverlayMixInterpSpeed = 0.f;

private:
    UFUNCTION()
    void OnRep_IsInCombat();

    TMap<TWeakObjectPtr<APlayerState>, float> RecentAttackers;
};
