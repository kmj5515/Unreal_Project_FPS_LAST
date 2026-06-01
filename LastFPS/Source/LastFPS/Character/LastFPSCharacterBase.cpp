#include "Character/LastFPSCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LastFPSPlayerController.h"

ALastFPSCharacterBase::ALastFPSCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
}

UAbilitySystemComponent* ALastFPSCharacterBase::GetAbilitySystemComponent() const
{
    if (const ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>())
        return PS->GetAbilitySystemComponent();
    return nullptr;
}

bool ALastFPSCharacterBase::IsAlive() const
{
    return AttributeSet && AttributeSet->GetHealth() > 0.f;
}

float ALastFPSCharacterBase::GetHealth() const
{
    return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float ALastFPSCharacterBase::GetMaxHealth() const
{
    return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

FString ALastFPSCharacterBase::GetKillFeedDisplayName() const
{
    if (!CharacterNickname.IsEmpty())
    {
        return CharacterNickname;
    }

    if (const APlayerState* PS = GetPlayerState())
    {
        return PS->GetPlayerName();
    }

    return FString();
}

FString ALastFPSCharacterBase::GetKillFeedDisplayNameForPlayerState(const APlayerState* PS)
{
    if (!PS)
    {
        return FString();
    }

    if (const ALastFPSCharacterBase* Character = Cast<ALastFPSCharacterBase>(PS->GetPawn()))
    {
        return Character->GetKillFeedDisplayName();
    }

    return PS->GetPlayerName();
}

void ALastFPSCharacterBase::Multicast_PlayHitSound_Implementation()
{
    if (HitSound)
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
}

void ALastFPSCharacterBase::Client_NotifyHitMarker_Implementation()
{
    APlayerController* PC = GetController<APlayerController>();
    if (!PC) return;
    if (ALastFPSPlayerController* LastPC = Cast<ALastFPSPlayerController>(PC))
    {
        LastPC->ShowHitMarker();
    }
}

void ALastFPSCharacterBase::RecordAttacker(APlayerState* Attacker)
{
    if (!Attacker) return;
    const float Now = GetWorld()->GetTimeSeconds();
    for (auto It = RecentAttackers.CreateIterator(); It; ++It)
        if (Now - It->Value > AssistTimeWindow)
            It.RemoveCurrent();
    RecentAttackers.Add(TWeakObjectPtr<APlayerState>(Attacker), Now);
}

void ALastFPSCharacterBase::ClearRecentAttackers()
{
    RecentAttackers.Reset();
}

void ALastFPSCharacterBase::BeginPlay()
{
    Super::BeginPlay();
}

void ALastFPSCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HealthDelegateHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
                .Remove(HealthDelegateHandle);
        }
        HealthDelegateHandle.Reset();
    }

    if (MoveSpeedDelegateHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetMoveSpeedAttribute())
                .Remove(MoveSpeedDelegateHandle);
        }
        MoveSpeedDelegateHandle.Reset();
    }
    Super::EndPlay(EndPlayReason);
}

void ALastFPSCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitAbilitySystem();
}

void ALastFPSCharacterBase::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilitySystem();
}

void ALastFPSCharacterBase::InitAbilitySystem()
{
    ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>();
    if (!PS) return;

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    if (!ASC) return;

    // PlayerState가 GAS Owner, Character가 Avatar
    ASC->InitAbilityActorInfo(PS, this);

    // AttributeSet 캐싱 — GetHealth() 등이 PlayerState 캐스팅 없이 접근 가능
    AttributeSet = PS->GetAttributeSet();

    if (!MoveSpeedDelegateHandle.IsValid())
    {
        MoveSpeedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            ULastFPSAttributeSet::GetMoveSpeedAttribute())
            .AddUObject(this, &ALastFPSCharacterBase::OnMoveSpeedChanged);
    }

    if (!HealthDelegateHandle.IsValid())
    {
        HealthDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            ULastFPSAttributeSet::GetHealthAttribute())
            .AddUObject(this, &ALastFPSCharacterBase::OnHealthChanged);
    }

    if (HasAuthority() && !PS->HasGrantedGASDefaults())
    {
        GiveDefaultAbilities();
        ApplyDefaultEffects();
        PS->MarkGASDefaultsGranted();
    }

    if (AttributeSet && GetCharacterMovement())
        GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed();

    UpdateAliveCollisionState(IsAlive());
}

void ALastFPSCharacterBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    UpdateAliveCollisionState(Data.NewValue > 0.f);
}

void ALastFPSCharacterBase::UpdateAliveCollisionState(bool bAlive)
{
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(bAlive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetCollisionEnabled(bAlive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        if (bAlive)
        {
            MoveComp->SetMovementMode(MOVE_Walking);
        }
        else
        {
            // 사망 시 이동/중력 업데이트를 멈춰 시체가 바닥 아래로 내려가지 않게 고정
            MoveComp->StopMovementImmediately();
            MoveComp->DisableMovement();
        }
    }
}

void ALastFPSCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
    GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void ALastFPSCharacterBase::GiveDefaultAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!HasAuthority() || !ASC) return;

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
            ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
    }
}

void ALastFPSCharacterBase::ApplyDefaultEffects()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!HasAuthority() || !ASC) return;

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(this);

    for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultEffects)
    {
        if (!EffectClass) continue;
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
        if (Spec.IsValid())
            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}
