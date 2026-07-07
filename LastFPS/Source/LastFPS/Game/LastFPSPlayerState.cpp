#include "Game/LastFPSPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

ALastFPSPlayerState::ALastFPSPlayerState()
{
    // PlayerState 복제 갱신 빈도 — 기본값(1)보다 높여 GAS 응답성 확보
    SetNetUpdateFrequency(100.f);

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    // Mixed: GE는 소유 클라이언트에만, GameplayCue는 모든 클라이언트에 복제
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("AttributeSet"));
    AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
}

void ALastFPSPlayerState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // SetIsReplicatedByDefault는 protected라 외부 호출 불가.
    // 컴포넌트 등록/초기화 이후인 이 시점에서 SetIsReplicated를 호출하면 ensure가 발동하지 않음.
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetIsReplicated(true);
    }
}

void ALastFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALastFPSPlayerState, StatKills);
    DOREPLIFETIME(ALastFPSPlayerState, StatDeaths);
    DOREPLIFETIME(ALastFPSPlayerState, StatAssists);
    DOREPLIFETIME(ALastFPSPlayerState, StatDamageDealt);
    DOREPLIFETIME(ALastFPSPlayerState, StatDamageTaken);
    DOREPLIFETIME(ALastFPSPlayerState, StatHealingReceived);
    DOREPLIFETIME(ALastFPSPlayerState, StatHealingGiven);
    DOREPLIFETIME(ALastFPSPlayerState, SelectedCharacterIndex);
}

UAbilitySystemComponent* ALastFPSPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ALastFPSPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    if (ALastFPSPlayerState* LastPlayerState = Cast<ALastFPSPlayerState>(PlayerState))
    {
        LastPlayerState->SelectedCharacterIndex = SelectedCharacterIndex;
    }
}

void ALastFPSPlayerState::OverrideWith(APlayerState* PlayerState)
{
    Super::OverrideWith(PlayerState);

    if (const ALastFPSPlayerState* LastPlayerState = Cast<ALastFPSPlayerState>(PlayerState))
    {
        SelectedCharacterIndex = LastPlayerState->SelectedCharacterIndex;
    }
}

void ALastFPSPlayerState::Auth_AddFloatStat(float& Stat, float Amount)
{
    if (!HasAuthority() || Amount <= 0.f) return;
    Stat += Amount;
}

void ALastFPSPlayerState::Auth_AddDamageDealt(
    float Amount,
    const FVector& DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    if (!HasAuthority() || Amount <= 0.f)
    {
        return;
    }

    const float OldValue = StatDamageDealt;
    Auth_AddFloatStat(StatDamageDealt, Amount);

    const float DeltaDamage = StatDamageDealt - OldValue;
    if (DeltaDamage > 0.f)
    {
        if (GetNetMode() == NM_Standalone)
        {
            OnDamageDealt.Broadcast(DeltaDamage, StatDamageDealt, DamageWorldLocation, DamageTargetActor, bCriticalHit);
        }
        else
        {
            Client_NotifyDamageDealt(DeltaDamage, StatDamageDealt, DamageWorldLocation, DamageTargetActor, bCriticalHit);
        }
    }
}

void ALastFPSPlayerState::Auth_GrantItem(FName ItemRowId, int32 Count)
{
    if (!HasAuthority() || ItemRowId.IsNone() || Count <= 0)
        return;

    if (GetNetMode() == NM_Standalone)
        GrantItemLocal(ItemRowId, Count);
    else
        Client_GrantItem(ItemRowId, Count);   // 호스트 자기 폰이면 로컬 실행됨
}

void ALastFPSPlayerState::Client_GrantItem_Implementation(FName ItemRowId, int32 Count)
{
    GrantItemLocal(ItemRowId, Count);
}

void ALastFPSPlayerState::GrantItemLocal(FName ItemRowId, int32 Count)
{
    if (UGameInstance* GI = GetGameInstance())
        if (ULastFPSEconomySubsystem* Econ = GI->GetSubsystem<ULastFPSEconomySubsystem>())
            Econ->AddItem(ItemRowId, Count);
}

void ALastFPSPlayerState::Auth_AddDamageTaken(float Amount)   { Auth_AddFloatStat(StatDamageTaken,     Amount); }
void ALastFPSPlayerState::Auth_AddHealingReceived(float Amount) { Auth_AddFloatStat(StatHealingReceived, Amount); }
void ALastFPSPlayerState::Auth_AddHealingGiven(float Amount)  { Auth_AddFloatStat(StatHealingGiven,    Amount); }

void ALastFPSPlayerState::Client_NotifyDamageDealt_Implementation(
    float DamageAmount,
    float TotalDamageDealt,
    FVector DamageWorldLocation,
    AActor* DamageTargetActor,
    bool bCriticalHit)
{
    if (DamageAmount > 0.f)
    {
        OnDamageDealt.Broadcast(DamageAmount, TotalDamageDealt, DamageWorldLocation, DamageTargetActor, bCriticalHit);
    }
}

void ALastFPSPlayerState::Auth_AddKill()
{
    if (!HasAuthority())
        return;
    ++StatKills;
}

void ALastFPSPlayerState::Auth_AddDeath()
{
    if (!HasAuthority())
        return;
    ++StatDeaths;
}

void ALastFPSPlayerState::Auth_AddAssist()
{
    if (!HasAuthority())
        return;
    ++StatAssists;
}

void ALastFPSPlayerState::Auth_SetSelectedCharacterIndex(int32 NewIndex)
{
    if (!HasAuthority())
        return;

    SelectedCharacterIndex = FMath::Max(0, NewIndex);
}
