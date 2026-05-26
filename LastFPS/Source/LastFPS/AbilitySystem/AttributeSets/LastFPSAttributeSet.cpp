#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "Character/LastFPSCharacterBase.h"
#include "Engine/World.h"
#include "Game/LastFPSPlayerState.h"
#include "Game/LastFPSMatchGameState.h"
#include "GameFramework/Pawn.h"

namespace
{
ALastFPSPlayerState* ResolveInstigatorPlayerState(const FGameplayEffectModCallbackData& Data)
{
    const FGameplayEffectContextHandle& Ctx = Data.EffectSpec.GetEffectContext();
    if (APawn* PawnInstigator = Cast<APawn>(Ctx.GetInstigator()))
    {
        if (ALastFPSPlayerState* PS = PawnInstigator->GetPlayerState<ALastFPSPlayerState>())
        {
            return PS;
        }
    }

    if (AActor* Causer = Ctx.GetEffectCauser())
    {
        if (APawn* PawnCauser = Cast<APawn>(Causer))
        {
            return PawnCauser->GetPlayerState<ALastFPSPlayerState>();
        }
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
    InitUltimateGauge(0.f);
    InitMaxUltimateGauge(static_cast<float>(ALastFPSPlayerState::UltimateKillsRequired));
    InitAttackDamage(10.f);
    InitDefense(0.f);
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
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, UltimateGauge,    COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, MaxUltimateGauge, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, AttackDamage,     COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, Defense,          COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ULastFPSAttributeSet, MoveSpeed,        COND_None, REPNOTIFY_Always);
}

void ULastFPSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    else if (Attribute == GetStaminaAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
    else if (Attribute == GetUltimateGaugeAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxUltimateGauge());
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

    if (TargetChar && AttackerPS && AttackerPS != VictimPS && ActualDamage > 0.f)
        TargetChar->RecordAttacker(AttackerPS);

    if (!VictimPS)
        return;

    VictimPS->Auth_AddDamageTaken(ActualDamage);
    if (AttackerPS && AttackerPS != VictimPS)
        AttackerPS->Auth_AddDamageDealt(ActualDamage);

    if (NewHealth > KINDA_SMALL_NUMBER || ActualDamage <= 0.f)
        return;

    VictimPS->Auth_AddDeath();
    if (AttackerPS && AttackerPS != VictimPS)
    {
        AttackerPS->Auth_AddKill();
        AttackerPS->Auth_OnScoredKill(VictimPS);

        if (ALastFPSMatchGameState* MatchGS = GetWorld()->GetGameState<ALastFPSMatchGameState>())
        {
            MatchGS->Auth_BroadcastKillFeed(AttackerPS, VictimPS);
        }
    }

    if (!TargetChar)
        return;

    const float Now = GetWorld()->GetTimeSeconds();
    // 스냅샷: 이터레이션 중 ClearRecentAttackers 호출로 인한 데이터 유실 방지
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
void ULastFPSAttributeSet::OnRep_UltimateGauge(const FGameplayAttributeData& Old)    { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, UltimateGauge, Old); }
void ULastFPSAttributeSet::OnRep_MaxUltimateGauge(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, MaxUltimateGauge, Old); }
void ULastFPSAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& Old)     { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, AttackDamage, Old); }
void ULastFPSAttributeSet::OnRep_Defense(const FGameplayAttributeData& Old)          { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, Defense, Old); }
void ULastFPSAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& Old)        { GAMEPLAYATTRIBUTE_REPNOTIFY(ULastFPSAttributeSet, MoveSpeed, Old); }
