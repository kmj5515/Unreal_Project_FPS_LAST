#include "Character/LastFPSCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/LastFPSAbilitySet.h"
#include "Data/Characters/LastFPSCharacterVisualData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Game/LastFPSGameModeBase.h"
#include "Inventory/LastFPSLoadoutSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LastFPSPlayerController.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ALastFPSCharacterBase::ALastFPSCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    OwnedAbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("OwnedAbilitySystemComponent"));
    OwnedAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    OwnedAttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("OwnedAttributeSet"));
    OwnedAbilitySystemComponent->AddAttributeSetSubobject(OwnedAttributeSet.Get());
}

void ALastFPSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSCharacterBase, bIsInCombat);
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

float ALastFPSCharacterBase::GetAttackRange() const
{
    return AttributeSet ? AttributeSet->GetAttackRange() : 0.f;
}

const ULastFPSCharacterDefinition* ALastFPSCharacterBase::GetCharacterDefinition() const
{
    return ResolveCharacterDefinition();
}

void ALastFPSCharacterBase::SetCharacterDefinitionForSpawn(ULastFPSCharacterDefinition* InDefinition)
{
    CharacterDefinition = InDefinition;
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

void ALastFPSCharacterBase::MarkCombatEngaged()
{
    if (!IsAlive())
    {
        return;
    }

    if (!bIsInCombat)
    {
        bIsInCombat = true;
        OnCombatEngagedChanged();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(CombatEngagedTimerHandle);
    if (CombatEngagedDuration <= 0.f)
    {
        ClearCombatEngaged();
        return;
    }

    World->GetTimerManager().SetTimer(
        CombatEngagedTimerHandle,
        this,
        &ALastFPSCharacterBase::ClearCombatEngaged,
        CombatEngagedDuration,
        false);
}

void ALastFPSCharacterBase::ClearCombatEngaged()
{
    if (!bIsInCombat)
    {
        return;
    }

    bIsInCombat = false;
    OnCombatEngagedChanged();
}

void ALastFPSCharacterBase::OnRep_IsInCombat()
{
    OnCombatEngagedChanged();
}

void ALastFPSCharacterBase::OnCombatEngagedChanged()
{
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

void ALastFPSCharacterBase::Multicast_SetStatusOverlayMaterial_Implementation(
    UMaterialInterface* OverlayMaterial,
    FName MixParameterName,
    float MixValue,
    bool bInterpolateMix,
    float MixInterpSpeed)
{
    ApplyStatusOverlayMaterial(OverlayMaterial, MixParameterName, MixValue, bInterpolateMix, MixInterpSpeed);
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

void ALastFPSCharacterBase::HandleDeath()
{
    if (!HasAuthority() || bHasDied)
        return;

    bHasDied = true;
    OnDeath.Broadcast(this);
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

    if (UAbilitySystemComponent* ASC = BoundAttributeASC.Get())
    {
        UnbindStatusOverlayMaterials(ASC);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
        World->GetTimerManager().ClearTimer(CombatEngagedTimerHandle);
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
            UnbindStatusOverlayMaterials(PreviousASC);

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

    BindStatusOverlayMaterials(ASC);
    RefreshStatusOverlayMaterial();

    const bool bDefaultsAlreadyGranted = PS ? PS->HasGrantedGASDefaults() : bOwnedGASDefaultsGranted;
    if (HasAuthority() && !bDefaultsAlreadyGranted)
    {
        if (ALastFPSGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALastFPSGameModeBase>() : nullptr)
        {
            GM->ApplyCharacterDefinitionToAbilitySystem(ASC, ResolvedDefinition);
        }

        // 베이스 스탯 적용 직후, 장착 모듈 보정을 Infinite GE 로 얹는다 (서버 권위).
        if (UGameInstance* GI = GetGameInstance())
        {
            if (ULastFPSLoadoutSubsystem* Loadout = GI->GetSubsystem<ULastFPSLoadoutSubsystem>())
            {
                Loadout->ApplyToAbilitySystem(ASC);
            }
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
    if (!bAlive)
    {
        ClearCombatEngaged();
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(CombatEngagedTimerHandle);
        }
    }

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

void ALastFPSCharacterBase::BindStatusOverlayMaterials(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return;
    }

    if (!StatusOverlayConfig)
    {
        return;
    }

    for (const FLastFPSStatusOverlayMaterial& OverlayConfig : StatusOverlayConfig->OverlayMaterials)
    {
        if (!OverlayConfig.OverlayMaterial)
        {
            continue;
        }

        TArray<FGameplayTag> TagsToBind;
        if (OverlayConfig.StatusTag.IsValid())
        {
            TagsToBind.Add(OverlayConfig.StatusTag);
        }
        if (OverlayConfig.StackTag.IsValid())
        {
            TagsToBind.AddUnique(OverlayConfig.StackTag);
        }

        for (const FGameplayTag& TagToBind : TagsToBind)
        {
            if (StatusOverlayDelegateHandles.Contains(TagToBind))
            {
                continue;
            }

            FDelegateHandle DelegateHandle = ASC->RegisterGameplayTagEvent(
                TagToBind,
                EGameplayTagEventType::AnyCountChange)
                .AddUObject(this, &ALastFPSCharacterBase::OnStatusOverlayTagChanged);

            StatusOverlayDelegateHandles.Add(TagToBind, DelegateHandle);
        }
    }
}

void ALastFPSCharacterBase::UnbindStatusOverlayMaterials(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        StatusOverlayDelegateHandles.Reset();
        return;
    }

    for (const TPair<FGameplayTag, FDelegateHandle>& DelegatePair : StatusOverlayDelegateHandles)
    {
        ASC->RegisterGameplayTagEvent(DelegatePair.Key, EGameplayTagEventType::AnyCountChange)
            .Remove(DelegatePair.Value);
    }

    StatusOverlayDelegateHandles.Reset();
}

void ALastFPSCharacterBase::OnStatusOverlayTagChanged(FGameplayTag, int32)
{
    RefreshStatusOverlayMaterial();
}

void ALastFPSCharacterBase::RefreshStatusOverlayMaterial()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!ASC || !MeshComp)
    {
        return;
    }

    const FLastFPSStatusOverlayMaterial* BestOverlay = nullptr;
    if (StatusOverlayConfig)
    {
        for (const FLastFPSStatusOverlayMaterial& OverlayConfig : StatusOverlayConfig->OverlayMaterials)
        {
            if (!IsStatusOverlayActive(ASC, OverlayConfig))
            {
                continue;
            }

            if (!BestOverlay || OverlayConfig.Priority > BestOverlay->Priority)
            {
                BestOverlay = &OverlayConfig;
            }
        }
    }

    if (!BestOverlay)
    {
        ApplyStatusOverlayMaterial(nullptr, NAME_None, 0.f, false, 0.f);
        if (HasAuthority())
        {
            Multicast_SetStatusOverlayMaterial(nullptr, NAME_None, 0.f, false, 0.f);
        }
        return;
    }

    const float MixValue = GetStatusOverlayMix(ASC, *BestOverlay);
    ApplyStatusOverlayMaterial(
        BestOverlay->OverlayMaterial,
        BestOverlay->StackMixParameterName,
        MixValue,
        BestOverlay->bInterpolateStackMix,
        BestOverlay->StackMixInterpSpeed);
    if (HasAuthority())
    {
        Multicast_SetStatusOverlayMaterial(
            BestOverlay->OverlayMaterial,
            BestOverlay->StackMixParameterName,
            MixValue,
            BestOverlay->bInterpolateStackMix,
            BestOverlay->StackMixInterpSpeed);
    }
}

bool ALastFPSCharacterBase::IsStatusOverlayActive(
    UAbilitySystemComponent* ASC,
    const FLastFPSStatusOverlayMaterial& OverlayConfig) const
{
    if (!ASC || !OverlayConfig.OverlayMaterial)
    {
        return false;
    }

    return (OverlayConfig.StatusTag.IsValid() && ASC->GetTagCount(OverlayConfig.StatusTag) > 0)
        || GetStatusOverlayStackCount(ASC, OverlayConfig) > 0;
}

float ALastFPSCharacterBase::GetStatusOverlayMix(
    UAbilitySystemComponent* ASC,
    const FLastFPSStatusOverlayMaterial& OverlayConfig) const
{
    if (!ASC)
    {
        return 0.f;
    }

    if (OverlayConfig.StatusTag.IsValid() && ASC->GetTagCount(OverlayConfig.StatusTag) > 0)
    {
        return OverlayConfig.StackMixEndValue;
    }

    const int32 StackCount = GetStatusOverlayStackCount(ASC, OverlayConfig);
    const int32 StackCountForFullMix = GetStatusOverlayFullStackCount(OverlayConfig);
    const float StackAlpha = FMath::Clamp(
        static_cast<float>(StackCount) / static_cast<float>(StackCountForFullMix),
        0.f,
        1.f);
    return FMath::Lerp(OverlayConfig.StackMixStartValue, OverlayConfig.StackMixEndValue, StackAlpha);
}

int32 ALastFPSCharacterBase::GetStatusOverlayStackCount(
    UAbilitySystemComponent* ASC,
    const FLastFPSStatusOverlayMaterial& OverlayConfig) const
{
    if (!ASC || !OverlayConfig.StackTag.IsValid())
    {
        return 0;
    }

    FGameplayTagContainer StackTags;
    StackTags.AddTag(OverlayConfig.StackTag);
    const FGameplayEffectQuery StackQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StackTags);
    return ASC->GetAggregatedStackCount(StackQuery);
}

int32 ALastFPSCharacterBase::GetStatusOverlayFullStackCount(const FLastFPSStatusOverlayMaterial& OverlayConfig) const
{
    if (const UGameplayEffect* StackEffect = OverlayConfig.StackEffectClass.GetDefaultObject())
    {
        const int32 StackLimitCount = StackEffect->GetStackLimitCount();
        if (StackLimitCount > 0)
        {
            const int32 OverflowStepCount = StackEffect->OverflowEffects.IsEmpty() ? 0 : 1;
            return StackLimitCount + OverflowStepCount;
        }
    }

    return 1;
}

void ALastFPSCharacterBase::ApplyStatusOverlayMaterial(
    UMaterialInterface* OverlayMaterial,
    FName MixParameterName,
    float MixValue,
    bool bInterpolateMix,
    float MixInterpSpeed)
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!OverlayMaterial)
    {
        if (World)
        {
            World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
        }

        MeshComp->SetOverlayMaterial(nullptr);
        ActiveStatusOverlayMID = nullptr;
        ActiveStatusOverlaySourceMaterial = nullptr;
        ActiveStatusOverlayMixParameterName = NAME_None;
        ActiveStatusOverlayMixValue = 0.f;
        TargetStatusOverlayMixValue = 0.f;
        ActiveStatusOverlayMixInterpSpeed = 0.f;
        return;
    }

    const bool bCreateNewMaterial = !ActiveStatusOverlayMID || ActiveStatusOverlaySourceMaterial.Get() != OverlayMaterial;
    if (bCreateNewMaterial)
    {
        ActiveStatusOverlaySourceMaterial = OverlayMaterial;
        ActiveStatusOverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
        ActiveStatusOverlayMixValue = 0.f;
    }

    ActiveStatusOverlayMixParameterName = MixParameterName;
    TargetStatusOverlayMixValue = MixValue;
    ActiveStatusOverlayMixInterpSpeed = FMath::Max(0.f, MixInterpSpeed);

    if (ActiveStatusOverlayMID && !MixParameterName.IsNone())
    {
        if (bInterpolateMix
            && ActiveStatusOverlayMixInterpSpeed > 0.f
            && !FMath::IsNearlyEqual(ActiveStatusOverlayMixValue, TargetStatusOverlayMixValue, KINDA_SMALL_NUMBER)
            && World)
        {
            ActiveStatusOverlayMID->SetScalarParameterValue(MixParameterName, ActiveStatusOverlayMixValue);
            World->GetTimerManager().SetTimer(
                StatusOverlayMixInterpolationTimerHandle,
                this,
                &ALastFPSCharacterBase::UpdateStatusOverlayMixInterpolation,
                0.016f,
                true);
        }
        else
        {
            if (World)
            {
                World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
            }

            ActiveStatusOverlayMixValue = TargetStatusOverlayMixValue;
            ActiveStatusOverlayMID->SetScalarParameterValue(MixParameterName, ActiveStatusOverlayMixValue);
        }
    }
    else if (World)
    {
        World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
    }

    MeshComp->SetOverlayMaterial(ActiveStatusOverlayMID ? ActiveStatusOverlayMID.Get() : OverlayMaterial);
}

void ALastFPSCharacterBase::UpdateStatusOverlayMixInterpolation()
{
    UWorld* World = GetWorld();
    if (!World || !ActiveStatusOverlayMID || ActiveStatusOverlayMixParameterName.IsNone())
    {
        if (World)
        {
            World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
        }
        return;
    }

    const float DeltaTime = World->GetDeltaSeconds();
    ActiveStatusOverlayMixValue = FMath::FInterpTo(
        ActiveStatusOverlayMixValue,
        TargetStatusOverlayMixValue,
        DeltaTime,
        ActiveStatusOverlayMixInterpSpeed);

    if (FMath::IsNearlyEqual(ActiveStatusOverlayMixValue, TargetStatusOverlayMixValue, 0.001f))
    {
        ActiveStatusOverlayMixValue = TargetStatusOverlayMixValue;
        World->GetTimerManager().ClearTimer(StatusOverlayMixInterpolationTimerHandle);
    }

    ActiveStatusOverlayMID->SetScalarParameterValue(ActiveStatusOverlayMixParameterName, ActiveStatusOverlayMixValue);
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
        if (LastFPSDamage::IsDamageGameplayEffect(EffectClass))
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping damage GameplayEffect in startup effects: %s"), *GetNameSafe(EffectClass));
            continue;
        }

        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
        if (Spec.IsValid())
            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}
