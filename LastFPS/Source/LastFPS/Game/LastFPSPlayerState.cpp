#include "Game/LastFPSPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Character/LastFPSCharacterBase.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "Quest/LastFPSQuestSubsystem.h"
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
    DOREPLIFETIME(ALastFPSPlayerState, EquippedSlots);
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
        // 맵 이동(seamless travel)으로 PlayerState 가 새로 만들어져도 장비 구성이 유지돼야
        // 다음 맵에서 다시 제출을 기다리지 않는다.
        LastPlayerState->EquippedSlots = EquippedSlots;
    }
}

void ALastFPSPlayerState::OverrideWith(APlayerState* PlayerState)
{
    Super::OverrideWith(PlayerState);

    if (const ALastFPSPlayerState* LastPlayerState = Cast<ALastFPSPlayerState>(PlayerState))
    {
        SelectedCharacterIndex = LastPlayerState->SelectedCharacterIndex;
        EquippedSlots = LastPlayerState->EquippedSlots;
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

void ALastFPSPlayerState::Auth_SetEquippedSlots(const TArray<FLastFPSEquippedSlot>& NewSlots)
{
    if (!HasAuthority())
    {
        return;
    }

    // 형식 검증은 서버가 소유한 데이터 테이블로 한다. 클라이언트가 보낸 것은 아이템 행 ID 뿐이며
    // 스탯 수치는 서버가 다시 계산하므로, 조작해도 존재하는 아이템의 정상 수치를 벗어날 수 없다.
    // ponytail: 보유 수량은 클라이언트 Economy 에만 있어 검증 불가.
    //           서버 권위 인벤토리가 생기면 여기서 소유 검사도 함께 한다.
    // 조작된 클라이언트가 거대한 배열을 제출해 서버 메모리와 복제 대역폭을 먹지 못하게 막는다.
    // ponytail: 슬롯 종류별 합계를 계산하지 않고 넉넉한 상한 하나로 자른다.
    //           실제 구성은 이보다 훨씬 작으므로 정상 제출이 걸릴 일은 없다.
    constexpr int32 MaxSubmittedSlotCount = 64;
    if (NewSlots.Num() > MaxSubmittedSlotCount)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("%s: 장비 제출 개수가 상한(%d)을 넘어 무시합니다. Num=%d"),
            *GetPlayerName(), MaxSubmittedSlotCount, NewSlots.Num());
        return;
    }

    const UGameInstance* GameInstance = GetGameInstance();
    const ULastFPSEquipmentSubsystem* Equipment =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;

    EquippedSlots.Reset(NewSlots.Num());
    for (const FLastFPSEquippedSlot& Entry : NewSlots)
    {
        if (Equipment && !Equipment->IsSubmittedSlotWellFormed(Entry))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("%s: 형식이 맞지 않는 장비 제출을 무시합니다. Slot=%d Index=%d Item=%s"),
                *GetPlayerName(),
                static_cast<int32>(Entry.SlotType),
                Entry.SlotIndex,
                *Entry.ItemRowId.ToString());
            continue;
        }

        EquippedSlots.Add(Entry);
    }

    // 폰이 이미 스폰돼 있으면 즉시 반영한다. 제출은 클라이언트 접속 이후라
    // 폰 스폰보다 늦게 도착하는 것이 정상 경로다.
    if (ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(GetPawn()))
    {
        Character->Auth_RefreshEquipmentLoadout();
    }
}

void ALastFPSPlayerState::Auth_NotifyObjectiveKill(const FGameplayTag KillTag)
{
    if (!HasAuthority() || !KillTag.IsValid())
    {
        return;
    }

    // 파티 공용 퀘스트는 서버가 직접 GameState 에 합산한다 — 처치 통지가 킬러 소유 클라에만
    // 가면 파티원 기여가 합쳐지지 않는다. 아래 로컬 경로와 대상 퀘스트가 배타적이라 중복 집계는 없다.
    if (UGameInstance* GI = GetGameInstance())
        if (ULastFPSQuestSubsystem* Quest = GI->GetSubsystem<ULastFPSQuestSubsystem>())
            Quest->Auth_NotifyPartyObjectiveKill(KillTag);

    if (GetNetMode() == NM_Standalone)
        NotifyObjectiveKillLocal(KillTag);
    else
        Client_NotifyObjectiveKill(KillTag);   // 호스트 자기 폰이면 로컬 실행됨
}

void ALastFPSPlayerState::Client_NotifyObjectiveKill_Implementation(const FGameplayTag KillTag)
{
    NotifyObjectiveKillLocal(KillTag);
}

void ALastFPSPlayerState::NotifyObjectiveKillLocal(const FGameplayTag KillTag)
{
    if (UGameInstance* GI = GetGameInstance())
        if (ULastFPSQuestSubsystem* Quest = GI->GetSubsystem<ULastFPSQuestSubsystem>())
            Quest->NotifyObjectiveKill(KillTag);
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
