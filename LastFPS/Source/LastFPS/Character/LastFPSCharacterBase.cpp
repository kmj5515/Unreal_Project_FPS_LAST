#include "Character/LastFPSCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/LastFPSAbilitySet.h"
#include "Character/LastFPSCharacterVisualData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Game/LastFPSCharacterDefinition.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Game/LastFPSGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LastFPSPlayerController.h"

ALastFPSCharacterBase::ALastFPSCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    OwnedAbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("OwnedAbilitySystemComponent"));
    OwnedAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    OwnedAttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("OwnedAttributeSet"));
    OwnedAbilitySystemComponent->AddAttributeSetSubobject(OwnedAttributeSet.Get());
}

UAbilitySystemComponent* ALastFPSCharacterBase::GetAbilitySystemComponent() const
{
    if (const ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>())
        return PS->GetAbilitySystemComponent();

    return OwnedAbilitySystemComponent;
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

const ULastFPSCharacterDefinition* ALastFPSCharacterBase::GetCharacterDefinition() const
{
    return ResolveCharacterDefinition();
}

const ULastFPSCharacterDefinition* ALastFPSCharacterBase::ResolveCharacterDefinition() const
{
    if (CharacterDefinition)
    {
        return CharacterDefinition;
    }

    if (const ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>())
    {
        if (const UWorld* World = GetWorld())
        {
            if (const ALastFPSGameModeBase* GM = World->GetAuthGameMode<ALastFPSGameModeBase>())
            {
                return GM->GetCharacterDefinitionForIndex(PS->GetSelectedCharacterIndex());
            }
        }
    }

    if (const ALastFPSPlayerController* LastPC = Cast<ALastFPSPlayerController>(GetController()))
    {
        return LastPC->GetSelectedCharacterDefinition();
    }

    return nullptr;
}

void ALastFPSCharacterBase::ApplyCharacterVisuals(const ULastFPSCharacterDefinition* Definition)
{
    if (!Definition || !Definition->VisualData)
    {
        return;
    }

    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        return;
    }

    if (Definition->VisualData->SkeletalMesh)
    {
        MeshComp->SetSkeletalMesh(Definition->VisualData->SkeletalMesh);
    }

    if (Definition->VisualData->AnimClass)
    {
        MeshComp->SetAnimInstanceClass(Definition->VisualData->AnimClass);
    }
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

    if (const ULastFPSCharacterDefinition* Definition = ResolveCharacterDefinition())
    {
        if (Definition->CharacterType != ELastFPSCharacterType::Player)
        {
            InitAbilitySystem();
        }
    }
}

void ALastFPSCharacterBase::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (OwnedAbilitySystemComponent)
    {
        OwnedAbilitySystemComponent->SetIsReplicated(true);
    }
}

void ALastFPSCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HealthDelegateHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = BoundAttributeASC.Get())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetHealthAttribute())
                .Remove(HealthDelegateHandle);
        }
        HealthDelegateHandle.Reset();
    }

    if (MoveSpeedDelegateHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = BoundAttributeASC.Get())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(ULastFPSAttributeSet::GetMoveSpeedAttribute())
                .Remove(MoveSpeedDelegateHandle);
        }
        MoveSpeedDelegateHandle.Reset();
    }
    BoundAttributeASC.Reset();
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
    UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : OwnedAbilitySystemComponent.Get();
    if (!ASC) return;

    // Players use PlayerState as owner; non-player characters own their ASC.
    AActor* AbilityOwner = PS ? static_cast<AActor*>(PS) : static_cast<AActor*>(this);
    ASC->InitAbilityActorInfo(AbilityOwner, this);

    // Cache the active AttributeSet so attribute helpers work for both ownership modes.
    AttributeSet = PS ? PS->GetAttributeSet() : OwnedAttributeSet.Get();
    const ULastFPSCharacterDefinition* ResolvedDefinition = ResolveCharacterDefinition();
    ApplyCharacterVisuals(ResolvedDefinition);

    if (BoundAttributeASC.Get() != ASC)
    {
        if (UAbilitySystemComponent* PreviousASC = BoundAttributeASC.Get())
        {
            if (MoveSpeedDelegateHandle.IsValid())
            {
                PreviousASC->GetGameplayAttributeValueChangeDelegate(
                    ULastFPSAttributeSet::GetMoveSpeedAttribute())
                    .Remove(MoveSpeedDelegateHandle);
                MoveSpeedDelegateHandle.Reset();
            }

            if (HealthDelegateHandle.IsValid())
            {
                PreviousASC->GetGameplayAttributeValueChangeDelegate(
                    ULastFPSAttributeSet::GetHealthAttribute())
                    .Remove(HealthDelegateHandle);
                HealthDelegateHandle.Reset();
            }
        }

        BoundAttributeASC = ASC;
    }

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

    const bool bDefaultsAlreadyGranted = PS ? PS->HasGrantedGASDefaults() : bOwnedGASDefaultsGranted;
    if (HasAuthority() && !bDefaultsAlreadyGranted)
    {
        if (ALastFPSGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALastFPSGameModeBase>() : nullptr)
        {
            GM->ApplyCharacterDefinitionToAbilitySystem(ASC, ResolvedDefinition);
        }

        GiveDefaultAbilities();
        ApplyDefaultEffects();

        if (PS)
        {
            PS->MarkGASDefaultsGranted();
        }
        else
        {
            bOwnedGASDefaultsGranted = true;
        }
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

    const ULastFPSCharacterDefinition* ResolvedDefinition = ResolveCharacterDefinition();
    const TArray<TSubclassOf<UGameplayAbility>>* AbilitiesToGrant = &DefaultAbilities;
    if (ResolvedDefinition && ResolvedDefinition->AbilitySet)
    {
        AbilitiesToGrant = &ResolvedDefinition->AbilitySet->GrantedAbilities;
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : *AbilitiesToGrant)
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

    const ULastFPSCharacterDefinition* ResolvedDefinition = ResolveCharacterDefinition();
    const TArray<TSubclassOf<UGameplayEffect>>* EffectsToApply = &DefaultEffects;
    if (ResolvedDefinition && ResolvedDefinition->AbilitySet)
    {
        EffectsToApply = &ResolvedDefinition->AbilitySet->StartupEffects;
    }

    for (const TSubclassOf<UGameplayEffect>& EffectClass : *EffectsToApply)
    {
        if (!EffectClass) continue;
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
        if (Spec.IsValid())
            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}
