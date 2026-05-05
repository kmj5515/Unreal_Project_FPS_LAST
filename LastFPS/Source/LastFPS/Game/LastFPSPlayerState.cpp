#include "Game/LastFPSPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Net/UnrealNetwork.h"

ALastFPSPlayerState::ALastFPSPlayerState()
{
    // PlayerState 복제 갱신 빈도 — 기본값(1)보다 높여 GAS 응답성 확보
    NetUpdateFrequency = 100.f;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    // Mixed: GE는 소유 클라이언트에만, GameplayCue는 모든 클라이언트에 복제
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("AttributeSet"));
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
}

UAbilitySystemComponent* ALastFPSPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ALastFPSPlayerState::Auth_AddDamageDealt(float Amount)
{
    if (!HasAuthority() || Amount <= 0.f)
        return;
    StatDamageDealt += Amount;
}

void ALastFPSPlayerState::Auth_AddDamageTaken(float Amount)
{
    if (!HasAuthority() || Amount <= 0.f)
        return;
    StatDamageTaken += Amount;
}

void ALastFPSPlayerState::Auth_AddHealingReceived(float Amount)
{
    if (!HasAuthority() || Amount <= 0.f)
        return;
    StatHealingReceived += Amount;
}

void ALastFPSPlayerState::Auth_AddHealingGiven(float Amount)
{
    if (!HasAuthority() || Amount <= 0.f)
        return;
    StatHealingGiven += Amount;
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

