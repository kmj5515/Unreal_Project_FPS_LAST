#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "Character/LastFPSCharacterBase.h"
#include "Engine/World.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Utility/LastFPSTags.h"

namespace
{
ALastFPSCharacterBase* ResolvePlayerStateCharacter(const ALastFPSPlayerState* PlayerState)
{
    return PlayerState ? Cast<ALastFPSCharacterBase>(PlayerState->GetPawn()) : nullptr;
}

AActor* ResolveDamageSourceActor(const FGameplayEffectModCallbackData& Data)
{
    const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
    AActor* const Candidates[] = {
        Context.GetOriginalInstigator(),
        Context.GetInstigator(),
        Context.GetEffectCauser()
    };

    for (AActor* Candidate : Candidates)
    {
        if (!Candidate)
        {
            continue;
        }

        if (const APlayerState* PlayerState = Cast<APlayerState>(Candidate))
        {
            if (APawn* Pawn = PlayerState->GetPawn())
            {
                return Pawn;
            }
        }

        if (const AController* Controller = Cast<AController>(Candidate))
        {
            if (APawn* Pawn = Controller->GetPawn())
            {
                return Pawn;
            }
        }

        if (APawn* InstigatorPawn = Candidate->GetInstigator())
        {
            return InstigatorPawn;
        }

        return Candidate;
    }

    return nullptr;
}

// 데미지 컨텍스트의 instigator 가 PlayerState/Controller/Pawn 어느 형태든 공격자 Pawn 으로 정규화한 뒤
// 그 Pawn 의 PlayerState 를 반환한다. (ASC 가 PlayerState 소유라 컨텍스트 instigator 가 Pawn 이 아닐 수 있음)
ALastFPSPlayerState* ResolveInstigatorPlayerState(const FGameplayEffectModCallbackData& Data)
{
    if (const APawn* SourcePawn = Cast<APawn>(ResolveDamageSourceActor(Data)))
    {
        return SourcePawn->GetPlayerState<ALastFPSPlayerState>();
    }
    return nullptr;
}
}

ULastFPSAttributeSet::ULastFPSAttributeSet()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
    InitStamina(100.f);
    InitMaxStamina(100.f);
    InitAttackDamage(10.f);
    InitCriticalChance(0.f);
    InitCriticalDamagePercent(150.f);
    InitDefense(0.f);
    InitAttackRange(200.f);
    InitPhysicalDamageMultiplier(1.f);
    InitFireDamageMultiplier(1.f);
    InitIceDamageMultiplier(1.f);
    InitElectricDamageMultiplier(1.f);
    InitPoisonDamageMultiplier(1.f);
    InitMoveSpeed(500.f);
    InitDamage(0.f);
}

void ULastFPSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, Health,           COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, MaxHealth,        COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, Stamina,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, MaxStamina,       COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, AttackDamage,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, CriticalChance,   COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, CriticalDamagePercent, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, Defense,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, AttackRange,      COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, PhysicalDamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, FireDamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, IceDamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, ElectricDamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, PoisonDamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, MoveSpeed,        COND_None, REPNOTIFY_Always);
}

void ULastFPSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    else if (Attribute == GetStaminaAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
    else if (Attribute == GetCriticalChanceAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
    else if (Attribute == GetCriticalDamagePercentAttribute())
        NewValue = FMath::Max(100.f, NewValue);
    else if (Attribute == GetPhysicalDamageMultiplierAttribute()
        || Attribute == GetFireDamageMultiplierAttribute()
        || Attribute == GetIceDamageMultiplierAttribute()
        || Attribute == GetElectricDamageMultiplierAttribute()
        || Attribute == GetPoisonDamageMultiplierAttribute())
        NewValue = FMath::Max(0.f, NewValue);
    else if (Attribute == GetMoveSpeedAttribute())
        NewValue = FMath::Max(0.f, NewValue);
    else if (Attribute == GetAttackRangeAttribute())
        NewValue = FMath::Max(0.f, NewValue);
}

void ULastFPSAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (!GetOwningAbilitySystemComponent())
        return;

    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        HandleDamageEffect(Data);
        return;
    }

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
        HandleHealEffect(Data);
}

void ULastFPSAttributeSet::HandleDamageEffect(const FGameplayEffectModCallbackData& Data)
{
    const float Applied      = GetDamage();
    SetDamage(0.f);

    const float OldHealth    = GetHealth();
    const float NewHealth    = FMath::Clamp(OldHealth - Applied, 0.f, GetMaxHealth());
    const float ActualDamage = OldHealth - NewHealth;
    SetHealth(NewHealth);

    ALastFPSCharacterBase* TargetChar = Cast<ALastFPSCharacterBase>(Data.Target.GetAvatarActor());
    if (Applied > 0.f && ActualDamage > 0.f && TargetChar)
        TargetChar->Multicast_PlayHitSound();

    UAbilitySystemComponent* Asc = GetOwningAbilitySystemComponent();
    AActor* OwnerActor = Asc ? Asc->GetOwnerActor() : nullptr;
    if (!OwnerActor || !OwnerActor->HasAuthority())
        return;

    ALastFPSPlayerState* VictimPS   = Cast<ALastFPSPlayerState>(OwnerActor);
    ALastFPSPlayerState* AttackerPS = ResolveInstigatorPlayerState(Data);
    const bool bCriticalHit = Data.EffectSpec.GetSetByCallerMagnitude(
        LastFPSGameplayTags::SetByCaller_CriticalHit,
        false,
        0.f) > 0.5f;

    if (TargetChar && ActualDamage > 0.f)
    {
        TargetChar->MarkCombatEngaged();
        if (TargetChar->IsPlayerControlled())
        {
            TargetChar->Client_PlayDamageCameraShake();
        }

        AActor* DamageSourceActor = ResolveDamageSourceActor(Data);
        if (DamageSourceActor && DamageSourceActor != TargetChar)
        {
            const FVector DamageSourceDirection =
                (DamageSourceActor->GetActorLocation() - TargetChar->GetActorLocation()).GetSafeNormal2D();
            if (!DamageSourceDirection.IsNearlyZero())
            {
                // UI는 공격자가 있는 방향을, 랙돌은 공격자로부터 밀려나는 반대 방향을 사용한다.
                TargetChar->SetLastDamageImpulseDirection(-DamageSourceDirection);
                if (TargetChar->IsPlayerControlled())
                {
                    TargetChar->Client_NotifyDamageDirection(DamageSourceDirection);
                }
            }
        }
    }

    if (ALastFPSCharacterBase* AttackerChar = ResolvePlayerStateCharacter(AttackerPS))
    {
        if (AttackerChar != TargetChar && ActualDamage > 0.f)
        {
            AttackerChar->MarkCombatEngaged();
        }
    }

    if (TargetChar && AttackerPS && AttackerPS != VictimPS && ActualDamage > 0.f)
        TargetChar->RecordAttacker(AttackerPS);

    if (AttackerPS && AttackerPS != VictimPS && ActualDamage > 0.f)
    {
        const FVector DamageWorldLocation = TargetChar
            ? TargetChar->GetActorLocation()
            : FVector::ZeroVector;
        AttackerPS->Auth_AddDamageDealt(ActualDamage, DamageWorldLocation, TargetChar, bCriticalHit);
    }

    // HP가 0에 도달하면 사망 훅 1회 호출 (PlayerState 유무와 무관 — 드랍/미션/퀘스트 등 구독자에게 브로드캐스트).
    // 처치 목표(KillTarget) 통지도 HandleDeath 내부에서 GameInstance 퀘스트 서브시스템으로 직접 나간다.
    if (TargetChar && NewHealth <= KINDA_SMALL_NUMBER && ActualDamage > 0.f)
        TargetChar->HandleDeath(AttackerPS != VictimPS ? AttackerPS : nullptr);

    if (!VictimPS)
        return;

    VictimPS->Auth_AddDamageTaken(ActualDamage);

    if (NewHealth > KINDA_SMALL_NUMBER || ActualDamage <= 0.f)
        return;

    VictimPS->Auth_AddDeath();
    if (AttackerPS && AttackerPS != VictimPS)
    {
        AttackerPS->Auth_AddKill();
    }

    if (!TargetChar)
        return;

    const float Now = GetWorld()->GetTimeSeconds();
    const TMap<TWeakObjectPtr<APlayerState>, float> AttackersCopy = TargetChar->GetRecentAttackers();
    for (const auto& Pair : AttackersCopy)
    {
        if (!Pair.Key.IsValid()) continue;
        ALastFPSPlayerState* AssistPS = Cast<ALastFPSPlayerState>(Pair.Key.Get());
        if (!AssistPS || AssistPS == AttackerPS || AssistPS == VictimPS) continue;
        if (Now - Pair.Value <= ALastFPSCharacterBase::AssistTimeWindow)
            AssistPS->Auth_AddAssist();
    }
    TargetChar->ClearRecentAttackers();
}

void ULastFPSAttributeSet::HandleHealEffect(const FGameplayEffectModCallbackData& Data)
{
    const float Mag = Data.EvaluatedData.Magnitude;
    if (Mag <= 0.f)
        return;

    UAbilitySystemComponent* Asc = GetOwningAbilitySystemComponent();
    AActor* OwnerActor = Asc ? Asc->GetOwnerActor() : nullptr;
    if (!OwnerActor || !OwnerActor->HasAuthority())
        return;

    ALastFPSPlayerState* TargetPS = Cast<ALastFPSPlayerState>(OwnerActor);
    if (!TargetPS)
        return;

    TargetPS->Auth_AddHealingReceived(Mag);

    if (ALastFPSPlayerState* HealerPS = ResolveInstigatorPlayerState(Data))
    {
        if (HealerPS != TargetPS)
            HealerPS->Auth_AddHealingGiven(Mag);
    }
}

void ULastFPSAttributeSet::OnRep_Health(const FGameplayAttributeData& Old)           { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, Health, Old); }
void ULastFPSAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& Old)        { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, MaxHealth, Old); }
void ULastFPSAttributeSet::OnRep_Stamina(const FGameplayAttributeData& Old)          { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, Stamina, Old); }
void ULastFPSAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& Old)       { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, MaxStamina, Old); }
void ULastFPSAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& Old)     { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, AttackDamage, Old); }
void ULastFPSAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& Old)   { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, CriticalChance, Old); }
void ULastFPSAttributeSet::OnRep_CriticalDamagePercent(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, CriticalDamagePercent, Old); }
void ULastFPSAttributeSet::OnRep_Defense(const FGameplayAttributeData& Old)          { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, Defense, Old); }
void ULastFPSAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& Old)      { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, AttackRange, Old); }
void ULastFPSAttributeSet::OnRep_PhysicalDamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, PhysicalDamageMultiplier, Old); }
void ULastFPSAttributeSet::OnRep_FireDamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, FireDamageMultiplier, Old); }
void ULastFPSAttributeSet::OnRep_IceDamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, IceDamageMultiplier, Old); }
void ULastFPSAttributeSet::OnRep_ElectricDamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, ElectricDamageMultiplier, Old); }
void ULastFPSAttributeSet::OnRep_PoisonDamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, PoisonDamageMultiplier, Old); }
void ULastFPSAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& Old)        { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, MoveSpeed, Old); }
