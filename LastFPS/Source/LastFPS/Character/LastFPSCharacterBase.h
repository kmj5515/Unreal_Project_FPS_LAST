#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "LastFPSCharacterBase.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class ULastFPSCharacterDefinition;
class ULastFPSCharacterVisualData;
struct FStreamableHandle;
class UGameplayEffect;
class UGameplayAbility;
class UCameraShakeBase;
class ULastFPSStatusAnimationComponent;
class ULastFPSStatusOverlayComponent;
class USoundBase;
class APlayerState;
class ALastFPSPlayerState;

class ALastFPSCharacterBase;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLastFPSCharacterDeath, ALastFPSCharacterBase* /*DeadChar*/);

/** 캐릭터 메시의 AnimInstance 가 새로 만들어졌음을 알린다. */
DECLARE_MULTICAST_DELEGATE(FOnLastFPSCharacterAnimInstanceRecreated);

UCLASS(Abstract)
class LASTFPS_API ALastFPSCharacterBase
    : public ACharacter
    , public IAbilitySystemInterface
    , public IGameplayTagAssetInterface
{
    GENERATED_BODY()

public:
    ALastFPSCharacterBase();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // IAbilitySystemInterface — PlayerState의 ASC를 반환
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    /** 캐릭터 분류 태그와 ASC 상태 태그를 표준 Gameplay Tag 인터페이스로 노출한다. */
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

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

    /** 런타임 캐릭터 정의에 지정된 분류 태그를 조회한다. */
    UFUNCTION(BlueprintPure, Category="LastFPS|Character", meta=(Categories="Character.Type"))
    bool HasCharacterClassificationTag(FGameplayTag TagToCheck) const;

    /** 이 캐릭터를 처치했을 때 KillTarget 퀘스트에 통지할 종류 태그. 비면 통지 안 함(플레이어 등). */
    UFUNCTION(BlueprintPure, Category="LastFPS|Quest")
    FGameplayTag GetQuestKillTag() const { return QuestKillTag; }

    void SetCharacterDefinitionForSpawn(ULastFPSCharacterDefinition* InDefinition);

    /**
     * BeginPlay가 끝난 풀 Actor에 새 정의와 초기 상태를 다시 적용한다.
     * 아직 BeginPlay 전인 Deferred Spawn에서는 정의만 저장하고 기존 초기화 흐름에 맡긴다.
     */
    virtual void ResetForPoolReuse(ULastFPSCharacterDefinition* InDefinition);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayHitSound();

    /** 서버가 판정한 공격자의 월드 방향을 소유 클라이언트 HUD에 전달한다. */
    UFUNCTION(Client, Reliable)
    void Client_NotifyDamageDirection(FVector_NetQuantizeNormal DamageSourceDirection);

    /** 서버가 확정한 체력 피해의 공통 카메라 피드백을 소유 클라이언트에서 재생한다. */
    UFUNCTION(Client, Unreliable)
    void Client_PlayDamageCameraShake();

    // ── 어시스트 추적 (서버 전용) ──────────────────────────────
    static constexpr float AssistTimeWindow = 10.f;

    void RecordAttacker(APlayerState* Attacker);
    void ClearRecentAttackers();
    const TMap<TWeakObjectPtr<APlayerState>, float>& GetRecentAttackers() const { return RecentAttackers; }

    /** 마지막 피해가 밀어내는 월드 방향을 기록한다. 사망 랙돌 같은 공통 피드백이 사용한다. */
    void SetLastDamageImpulseDirection(const FVector& Direction);
    const FVector& GetLastDamageImpulseDirection() const { return LastDamageImpulseDirection; }

    /**
     * 서버: HP 0 도달 시 1회 호출되는 사망 훅. 래치되어 중복 호출은 무시된다.
     * @param KillerPlayerState 처치한 플레이어. 퀘스트 진행을 그 소유 클라이언트에 통지하는 데 쓴다.
     *                          환경 피해 등 가해자가 없으면 null.
     */
    void HandleDeath(ALastFPSPlayerState* KillerPlayerState = nullptr);

    /** 서버: PlayerState 의 장비 구성이 갱신됐을 때 폰 스탯을 다시 맞춘다. */
    void Auth_RefreshEquipmentLoadout();

    // 서버 사망 시 브로드캐스트. 드랍/미션 등이 구독한다.
    FOnLastFPSCharacterDeath OnDeath;

    /**
     * 스켈레탈 메시나 AnimBP 교체로 AnimInstance 가 재생성된 직후 브로드캐스트한다.
     * LinkAnimClassLayers 로 연결한 애님 레이어는 AnimInstance 와 함께 사라지므로,
     * 레이어를 소유한 컴포넌트가 이 시점에 다시 링크해야 레퍼런스 포즈가 노출되지 않는다.
     */
    FOnLastFPSCharacterAnimInstanceRecreated OnAnimInstanceRecreated;

    /**
     * 이 캐릭터의 정의. 아웃게임 UI 가 표시용 데이터(시각 데이터 등)를 읽을 때도 쓴다.
     * 조회 전용이라 공개해도 캐릭터의 상태를 밖에서 바꿀 수는 없다.
     */
    const ULastFPSCharacterDefinition* ResolveCharacterDefinition() const;

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
    virtual void UpdateAliveCollisionState(bool bAlive);
    virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
    virtual float ResolveMaxWalkSpeed(float AttributeMoveSpeed) const;
    void ApplyCharacterVisuals(const ULastFPSCharacterDefinition* Definition);

    /** VisualData 가 상주한 뒤 실제 메시·AnimBP 를 교체하는 본문이다. */
    void ApplyCharacterVisualsInternal(const ULastFPSCharacterVisualData* VisualData);

    /** 비주얼 애셋 비동기 로드 핸들. 로드 중 정의가 바뀌면 콜백에서 걸러낸다. */
    TSharedPtr<FStreamableHandle> VisualDataLoadHandle;

    /** 서버: PlayerState 가 소유한 장비 구성으로 보정 GE 를 다시 적용한다(이전 GE 제거 포함). */
    void ApplyEquipmentStats();

    /** 서버: 장비 구성 갱신 후 스탯 외 반영이 필요한 파생 클래스가 재정의한다(무기 로드아웃 등). */
    virtual void OnEquipmentLoadoutRefreshed();
    void ClearCombatEngaged();
    virtual void OnCombatEngagedChanged();

    // Character-owned GAS used when there is no PlayerState ASC.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> OwnedAbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<ULastFPSAttributeSet> OwnedAttributeSet;

	UPROPERTY()
	TObjectPtr<ULastFPSAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	TObjectPtr<ULastFPSStatusOverlayComponent> StatusOverlayComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LastFPS|Status Animation")
	TObjectPtr<ULastFPSStatusAnimationComponent> StatusAnimationComponent;

    bool bOwnedGASDefaultsGranted = false;
    bool bHasDied = false;
    FVector LastDamageImpulseDirection = FVector::ZeroVector;
    TWeakObjectPtr<UAbilitySystemComponent> BoundAttributeASC;

    /** BP/에디터에서 캐릭터별 닉네임 지정. 비어 있으면 GetPlayerName() */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Display")
    FString CharacterNickname;

    /**
     * 레벨에 직접 배치하거나 BP 기본값으로 종류를 고정하는 캐릭터의 정의다.
     * 복제 멤버 CharacterDefinition 은 Transient 라 저작 값을 담을 수 없고,
     * 배치형 캐릭터는 PlayerState·PlayerController 해석 경로가 모두 실패하므로
     * 이 저작 값이 없으면 정의 없이 스폰되어 스탯과 비주얼이 적용되지 않는다.
     * 런타임에 정의를 지정받는 캐릭터(인카운터 스폰 등)는 비워 둔다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Character")
    TSoftObjectPtr<ULastFPSCharacterDefinition> AuthoredCharacterDefinition;

    UPROPERTY(ReplicatedUsing=OnRep_CharacterDefinition, Transient)
    TObjectPtr<ULastFPSCharacterDefinition> CharacterDefinition;

    UFUNCTION()
    void OnRep_CharacterDefinition();

    /** 서버가 적용한 정의의 분류 태그를 클라이언트 HUD에서도 조회하기 위한 읽기 전용 복제 캐시다. */
    UPROPERTY(Replicated)
    FGameplayTagContainer ReplicatedClassificationTags;

    /** KillTarget 퀘스트용 종류 태그(예: Enemy.Type.Grunt). 적 BP 에서 설정, 플레이어는 비움. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Quest")
    FGameplayTag QuestKillTag;

    UPROPERTY(EditDefaultsOnly, Category="LastFPS|Sound")
    TObjectPtr<USoundBase> HitSound;

    /** 모든 실제 체력 피해에 공통으로 사용하는 피격 카메라 흔들림이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Feedback|Damage")
    TSubclassOf<UCameraShakeBase> DamageCameraShakeClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Feedback|Damage", meta=(ClampMin="0.0"))
    float DamageCameraShakeScale = 1.f;

    // 기본 어빌리티 목록 (에디터에서 할당)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 스폰 시 즉시 적용할 기본 Effect (초기 스탯 세팅용)
    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	FDelegateHandle MoveSpeedDelegateHandle;
	FDelegateHandle HealthDelegateHandle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Combat", meta=(ClampMin="0.0"))
    float CombatEngagedDuration = 3.f;

    UPROPERTY(ReplicatedUsing=OnRep_IsInCombat, BlueprintReadOnly, Category="LastFPS|Combat")
    bool bIsInCombat = false;

	FTimerHandle CombatEngagedTimerHandle;

private:
    /**
     * 서버가 해석한 정의를 복제 멤버로 확정한다.
     * 시뮬레이티드 프록시는 GameMode 와 Controller 에 접근할 수 없어
     * ResolveCharacterDefinition() 의 대체 경로가 모두 실패한다.
     * 서버가 값을 확정해 복제하지 않으면 프록시에서 메시와 AnimBP 가 적용되지 않는다.
     */
    void CommitCharacterDefinitionOnAuthority();

    UFUNCTION()
    void OnRep_IsInCombat();

    TMap<TWeakObjectPtr<APlayerState>, float> RecentAttackers;
};
