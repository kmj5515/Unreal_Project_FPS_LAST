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
class USoundBase;
class APlayerState;

UCLASS(Abstract)
class LASTFPS_API ALastFPSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ALastFPSCharacterBase();

    // IAbilitySystemInterface — PlayerState의 ASC를 반환
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    bool IsAlive() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetHealth() const;

    UFUNCTION(BlueprintCallable, Category="LastFPS|Attributes")
    float GetMaxHealth() const;

    /** 킬피드 등 UI 표시명. 비어 있으면 PlayerState 이름 사용 */
    UFUNCTION(BlueprintPure, Category="LastFPS|Display")
    FString GetKillFeedDisplayName() const;

    /** Pawn 없을 때 PlayerState만으로 표시명 해석 */
    static FString GetKillFeedDisplayNameForPlayerState(const APlayerState* PS);

    virtual bool GetIsADS() const { return false; }

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayHitSound();

    /** 서버: 명중 처리 후 발사자 클라이언트에서만 히트마커 표시 */
    UFUNCTION(Client, Reliable)
    void Client_NotifyHitMarker();

    // ── 어시스트 추적 (서버 전용) ──────────────────────────────
    static constexpr float AssistTimeWindow = 10.f;

    void RecordAttacker(APlayerState* Attacker);
    void ClearRecentAttackers();
    const TMap<TWeakObjectPtr<APlayerState>, float>& GetRecentAttackers() const { return RecentAttackers; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 서버: Pawn 빙의 시 ASC 초기화
    virtual void PossessedBy(AController* NewController) override;
    // 클라이언트: PlayerState 복제 완료 시 ASC 초기화
    virtual void OnRep_PlayerState() override;

    void InitAbilitySystem();
    virtual void GiveDefaultAbilities();
    void ApplyDefaultEffects();
    void OnHealthChanged(const FOnAttributeChangeData& Data);
    void UpdateAliveCollisionState(bool bAlive);
    void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);

    // AttributeSet은 PlayerState가 소유 — InitAbilitySystem에서 캐싱
    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> AttributeSet;

    /** BP/에디터에서 캐릭터별 닉네임 지정. 비어 있으면 GetPlayerName() */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Display")
    FString CharacterNickname;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Sound")
    TObjectPtr<USoundBase> HitSound;

    // 기본 어빌리티 목록 (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스폰 시 즉시 적용할 기본 Effect (초기 스탯 세팅용)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

    FDelegateHandle MoveSpeedDelegateHandle;
    FDelegateHandle HealthDelegateHandle;

private:
    TMap<TWeakObjectPtr<APlayerState>, float> RecentAttackers;
};
