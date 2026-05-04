#include "Character/LastFPSCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Game/LastFPSPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/LastFPSHUD.h"

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

void ALastFPSCharacterBase::Multicast_PlayHitSound_Implementation()
{
    if (HitSound)
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
}

void ALastFPSCharacterBase::Client_NotifyHitMarker_Implementation()
{
    APlayerController* PC = GetController<APlayerController>();
    if (!PC) return;
    if (ALastFPSHUD* FPSHUD = Cast<ALastFPSHUD>(PC->GetHUD()))
        FPSHUD->ShowHitMarker();
}

void ALastFPSCharacterBase::BeginPlay()
{
    Super::BeginPlay();
}

void ALastFPSCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

    if (HasAuthority() && !PS->HasGrantedGASDefaults())
    {
        GiveDefaultAbilities();
        ApplyDefaultEffects();
        PS->MarkGASDefaultsGranted();
    }

    if (AttributeSet && GetCharacterMovement())
        GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
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
