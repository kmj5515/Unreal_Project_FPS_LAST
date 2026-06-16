#include "Game/LastFPSPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Net/UnrealNetwork.h"
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

void ALastFPSPlayerState::Auth_AddDamageDealt(float Amount)   { Auth_AddFloatStat(StatDamageDealt,     Amount); }
void ALastFPSPlayerState::Auth_AddDamageTaken(float Amount)   { Auth_AddFloatStat(StatDamageTaken,     Amount); }
void ALastFPSPlayerState::Auth_AddHealingReceived(float Amount) { Auth_AddFloatStat(StatHealingReceived, Amount); }
void ALastFPSPlayerState::Auth_AddHealingGiven(float Amount)  { Auth_AddFloatStat(StatHealingGiven,    Amount); }

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
