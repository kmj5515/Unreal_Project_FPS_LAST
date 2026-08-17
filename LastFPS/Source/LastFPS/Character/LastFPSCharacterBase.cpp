#include "Character/LastFPSCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/LastFPSAbilitySet.h"
#include "Character/Components/LastFPSStatusAnimationComponent.h"
#include "Character/Components/LastFPSStatusOverlayComponent.h"
#include "Data/Characters/LastFPSCharacterVisualData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Game/LastFPSGameModeBase.h"
#include "Inventory/LastFPSEquipmentSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LastFPSPlayerController.h"
#include "Utility/LastFPSDamageCalculation.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Quest/LastFPSQuestSubsystem.h"

ALastFPSCharacterBase::ALastFPSCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    OwnedAbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("OwnedAbilitySystemComponent"));
    OwnedAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	OwnedAttributeSet = CreateDefaultSubobject<ULastFPSAttributeSet>(TEXT("OwnedAttributeSet"));
	OwnedAbilitySystemComponent->AddAttributeSetSubobject(OwnedAttributeSet.Get());

	StatusOverlayComponent = CreateDefaultSubobject<ULastFPSStatusOverlayComponent>(TEXT("StatusOverlayComponent"));
	StatusAnimationComponent = CreateDefaultSubobject<ULastFPSStatusAnimationComponent>(TEXT("StatusAnimationComponent"));
}

void ALastFPSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSCharacterBase, CharacterDefinition);
    DOREPLIFETIME(ALastFPSCharacterBase, bIsInCombat);
    DOREPLIFETIME(ALastFPSCharacterBase, ReplicatedClassificationTags);
}

UAbilitySystemComponent* ALastFPSCharacterBase::GetAbilitySystemComponent() const
{
    if (const ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>())
        return PS->GetAbilitySystemComponent();

    return OwnedAbilitySystemComponent;
}

void ALastFPSCharacterBase::GetOwnedGameplayTags(
    FGameplayTagContainer& TagContainer) const
{
    TagContainer = ReplicatedClassificationTags;
    if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        FGameplayTagContainer AbilitySystemTags;
        ASC->GetOwnedGameplayTags(AbilitySystemTags);
        TagContainer.AppendTags(AbilitySystemTags);
    }
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

bool ALastFPSCharacterBase::HasCharacterClassificationTag(const FGameplayTag TagToCheck) const
{
    if (!TagToCheck.IsValid())
    {
        return false;
    }

    if (ReplicatedClassificationTags.HasTag(TagToCheck))
    {
        return true;
    }

    const ULastFPSCharacterDefinition* Definition = ResolveCharacterDefinition();
    return Definition && Definition->HasClassificationTag(TagToCheck);
}

void ALastFPSCharacterBase::SetCharacterDefinitionForSpawn(ULastFPSCharacterDefinition* InDefinition)
{
    CharacterDefinition = InDefinition;
    ReplicatedClassificationTags = InDefinition
        ? InDefinition->ClassificationTags
        : FGameplayTagContainer();
}

void ALastFPSCharacterBase::ResetForPoolReuse(
    ULastFPSCharacterDefinition* InDefinition)
{
    SetCharacterDefinitionForSpawn(InDefinition);
    if (!HasActorBegunPlay())
    {
        return;
    }

    bHasDied = false;
    LastDamageImpulseDirection = FVector::ZeroVector;
    ClearRecentAttackers();
    ClearCombatEngaged();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CombatEngagedTimerHandle);
    }

    InitAbilitySystem();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (HasAuthority() && ASC)
    {
        ASC->CancelAllAbilities();
        ASC->ClearAllAbilities();
        ASC->RemoveActiveEffects(FGameplayEffectQuery());

        if (InDefinition)
        {
            InDefinition->GiveToAbilitySystem(ASC);
        }
        GiveDefaultAbilities();
        ApplyDefaultEffects();
    }

    if (AttributeSet && GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed =
            ResolveMaxWalkSpeed(AttributeSet->GetMoveSpeed());
    }

    ApplyCharacterVisuals(InDefinition);
    UpdateAliveCollisionState(IsAlive());
    ForceNetUpdate();
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

    // 플레이어 선택 경로가 모두 실패한 뒤에만 저작 값을 쓴다.
    // (선택된 캐릭터가 있는 플레이어 폰에서 BP 기본값이 선택을 덮지 않도록 마지막 순서에 둔다.)
    if (!AuthoredCharacterDefinition.IsNull())
    {
        return AuthoredCharacterDefinition.LoadSynchronous();
    }

    return nullptr;
}

void ALastFPSCharacterBase::ApplyCharacterVisuals(const ULastFPSCharacterDefinition* Definition)
{
    if (!Definition || Definition->VisualData.IsNull())
    {
        return;
    }

    // VisualData 는 스켈레탈 메시와 AnimBP 를 하드 참조로 물고 있어 로드 비용이 크다.
    // 동기 로드하면 다른 플레이어가 시야에 들어오는 순간 게임 스레드가 통째로 멈춘다.
    // 이미 메모리에 있으면 그대로 적용하고, 아니면 받아 둔 뒤 적용한다.
    if (ULastFPSCharacterVisualData* LoadedVisualData = Definition->VisualData.Get())
    {
        ApplyCharacterVisualsInternal(LoadedVisualData);
        return;
    }

    TWeakObjectPtr<ALastFPSCharacterBase> WeakThis(this);
    TWeakObjectPtr<const ULastFPSCharacterDefinition> WeakDefinition(Definition);
    VisualDataLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        Definition->VisualData.ToSoftObjectPath(),
        FStreamableDelegate::CreateLambda(
            [WeakThis, WeakDefinition]()
            {
                ALastFPSCharacterBase* Character = WeakThis.Get();
                const ULastFPSCharacterDefinition* ResolvedDefinition = WeakDefinition.Get();
                if (!Character || !ResolvedDefinition)
                {
                    return;
                }

                // 로드 중에 정의가 교체됐다면 낡은 비주얼을 덮어씌우지 않는다.
                if (Character->ResolveCharacterDefinition() != ResolvedDefinition)
                {
                    return;
                }

                Character->ApplyCharacterVisualsInternal(ResolvedDefinition->VisualData.Get());
            }),
        FStreamableManager::AsyncLoadHighPriority);
}

void ALastFPSCharacterBase::ApplyCharacterVisualsInternal(
    const ULastFPSCharacterVisualData* VisualData)
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp || !VisualData)
    {
        return;
    }

    // 메시·AnimBP 교체는 AnimInstance 를 다시 만든다. 실제로 바뀐 경우에만 재생성되므로
    // 구독자에게 헛된 재링크를 요구하지 않도록 변경 여부를 먼저 판단한다.
    const bool bMeshChanged =
        VisualData->SkeletalMesh != nullptr
        && MeshComp->GetSkeletalMeshAsset() != VisualData->SkeletalMesh.Get();
    const bool bAnimClassChanged =
        VisualData->AnimClass != nullptr
        && MeshComp->GetAnimClass() != VisualData->AnimClass.Get();

    if (bMeshChanged)
    {
        MeshComp->SetSkeletalMesh(VisualData->SkeletalMesh);
    }

    if (bAnimClassChanged)
    {
        MeshComp->SetAnimInstanceClass(VisualData->AnimClass);
    }

    if (bMeshChanged || bAnimClassChanged)
    {
        OnAnimInstanceRecreated.Broadcast();
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

void ALastFPSCharacterBase::Client_NotifyDamageDirection_Implementation(
    const FVector_NetQuantizeNormal DamageSourceDirection)
{
    ALastFPSPlayerController* LastPC = GetController<ALastFPSPlayerController>();
    if (!LastPC)
    {
        return;
    }

    LastPC->ShowDamageDirection(DamageSourceDirection);
}

void ALastFPSCharacterBase::Client_PlayDamageCameraShake_Implementation()
{
    APlayerController* PlayerController = GetController<APlayerController>();
    if (!PlayerController
        || !PlayerController->PlayerCameraManager
        || !DamageCameraShakeClass
        || DamageCameraShakeScale <= 0.f)
    {
        return;
    }

    PlayerController->PlayerCameraManager->StartCameraShake(
        DamageCameraShakeClass,
        DamageCameraShakeScale,
        ECameraShakePlaySpace::CameraLocal);
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

void ALastFPSCharacterBase::SetLastDamageImpulseDirection(const FVector& Direction)
{
    LastDamageImpulseDirection = Direction.GetSafeNormal2D();
}

void ALastFPSCharacterBase::HandleDeath(ALastFPSPlayerState* KillerPlayerState)
{
    if (!HasAuthority() || bHasDied)
        return;

    bHasDied = true;
    OnDeath.Broadcast(this);

    // 처치 목표(KillTarget) 통지 — 퀘스트 진행은 GameInstance 서브시스템이 소유하므로 프로세스마다 하나뿐이다.
    // 서버에서 직접 올리면 리슨 서버 호스트의 진행만 쌓이고 원격 플레이어는 아무것도 받지 못한다.
    // 킬러의 PlayerState 를 거쳐 그 소유 클라이언트에서 올린다. QuestKillTag 가 비면 서브시스템이 무시한다.
    if (KillerPlayerState)
    {
        KillerPlayerState->Auth_NotifyObjectiveKill(QuestKillTag);
    }
}

void ALastFPSCharacterBase::Auth_RefreshEquipmentLoadout()
{
    if (!HasAuthority())
    {
        return;
    }

    ApplyEquipmentStats();
    OnEquipmentLoadoutRefreshed();
}

void ALastFPSCharacterBase::OnEquipmentLoadoutRefreshed()
{
    // 기본 캐릭터는 스탯만 갖는다. 무기를 드는 파생 클래스가 재정의한다.
}

void ALastFPSCharacterBase::ApplyEquipmentStats()
{
    ALastFPSPlayerState* PS = GetPlayerState<ALastFPSPlayerState>();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!HasAuthority() || !PS || !ASC)
    {
        // 장비는 플레이어만 갖는다. PlayerState 가 없는 적·NPC 는 정의 스탯만으로 완결된다.
        return;
    }

    const UGameInstance* GameInstance = GetGameInstance();
    const ULastFPSEquipmentSubsystem* Equipment =
        GameInstance ? GameInstance->GetSubsystem<ULastFPSEquipmentSubsystem>() : nullptr;
    if (!Equipment)
    {
        return;
    }

    // 재적용 시 이전 GE 를 반드시 제거한다. 남겨두면 장비를 바꿀 때마다 보정이 중첩된다.
    if (PS->EquipmentStatsHandle.IsValid())
    {
        ASC->RemoveActiveGameplayEffect(PS->EquipmentStatsHandle);
        PS->EquipmentStatsHandle = FActiveGameplayEffectHandle();
    }

    PS->EquipmentStatsHandle =
        Equipment->ApplyStatTotalsToAbilitySystem(ASC, Equipment->ComputeTotalsForSlots(PS->GetEquippedSlots()));
}

void ALastFPSCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (const ULastFPSCharacterDefinition* Definition = ResolveCharacterDefinition())
    {
        if (HasAuthority())
        {
            ReplicatedClassificationTags = Definition->ClassificationTags;
        }

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatEngagedTimerHandle);
    }

    BoundAttributeASC.Reset();
    Super::EndPlay(EndPlayReason);
}

void ALastFPSCharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    // InitAbilitySystem() 이 비주얼 적용까지 수행하므로 정의 확정이 먼저 와야 한다.
    CommitCharacterDefinitionOnAuthority();
    InitAbilitySystem();
}

void ALastFPSCharacterBase::CommitCharacterDefinitionOnAuthority()
{
    // 스폰 시점에 정의를 받은 캐릭터(적 등)는 이미 확정돼 있으므로 건드리지 않는다.
    if (!HasAuthority() || CharacterDefinition)
    {
        return;
    }

    const ULastFPSCharacterDefinition* ResolvedDefinition = ResolveCharacterDefinition();
    if (!ResolvedDefinition)
    {
        return;
    }

    // 정의는 조회 전용으로 노출돼 해석 경로 전체가 const 포인터를 반환한다.
    // 대상은 런타임에 변경하지 않는 데이터 애셋이고 여기서도 값을 수정하지 않으므로,
    // 복제 프로퍼티에 담기 위한 const 제거를 이 지점으로 한정한다.
    SetCharacterDefinitionForSpawn(const_cast<ULastFPSCharacterDefinition*>(ResolvedDefinition));
    ForceNetUpdate();
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

	if (StatusOverlayComponent)
	{
		StatusOverlayComponent->InitializeWithAbilitySystem(ASC);
	}
	if (StatusAnimationComponent)
	{
		StatusAnimationComponent->InitializeWithAbilitySystem(ASC);
	}

    const bool bDefaultsAlreadyGranted = PS ? PS->HasGrantedGASDefaults() : bOwnedGASDefaultsGranted;
    if (HasAuthority())
    {
        ALastFPSGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALastFPSGameModeBase>() : nullptr;

        if (!bDefaultsAlreadyGranted)
        {
            if (!ResolvedDefinition)
            {
                // 정의가 없으면 StatData 가 적용되지 않아 AttributeSet 의 C++ 기본값(체력 100)으로 남는다.
                // 밸런스가 통째로 어긋나는 상황이라 조용히 넘어가지 않는다.
                UE_LOG(LogTemp, Error,
                    TEXT("%s: CharacterDefinition 을 해석하지 못해 기본 스탯을 적용하지 못했습니다. "
                         "레벨 배치 캐릭터라면 AuthoredCharacterDefinition 을 지정하십시오."),
                    *GetName());
            }
            else
            {
                // 스탯 적용은 GameMode 종류에 의존하지 않는다.
                // 정의별 차이는 GiveToAbilitySystem 의 virtual 이 담당하므로
                // (Hero/Enemy 정의가 override) GameMode 를 거칠 이유가 없고,
                // ALastFPSGameModeBase 가 아닌 맵에서도 스탯이 그대로 적용된다.
                ResolvedDefinition->GiveToAbilitySystem(ASC);
            }

            // 베이스 스탯 적용 직후, 장착 장비(모듈·리액터·외장 부품) 보정을 Infinite GE 로 얹는다 (서버 권위).
            // EquipmentSubsystem 이 모듈까지 합산하므로 LoadoutSubsystem 을 따로 적용하면 이중 반영된다.
            ApplyEquipmentStats();

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

        // 레벨 제한(예: 허브 내 전투 금지)은 맵마다 다르므로 항상 새로 적용/갱신한다.
        if (GM)
        {
            GM->ApplyLevelRestrictionsToAbilitySystem(ASC);
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
	if (StatusAnimationComponent)
	{
		if (bAlive)
		{
			StatusAnimationComponent->ResumeStatusAnimationResponses();
		}
		else
		{
			// 래그돌 물리 전환 전에 빙결로 고정한 애니메이션 포즈를 반드시 해제한다.
			StatusAnimationComponent->PrepareForPhysicsSimulation();
		}
	}

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
    GetCharacterMovement()->MaxWalkSpeed = ResolveMaxWalkSpeed(Data.NewValue);
}

float ALastFPSCharacterBase::ResolveMaxWalkSpeed(const float AttributeMoveSpeed) const
{
	return FMath::Max(AttributeMoveSpeed, 0.f);
}

void ALastFPSCharacterBase::GiveDefaultAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!HasAuthority() || !ASC) return;

    const ULastFPSCharacterDefinition* ResolvedDefinition = ResolveCharacterDefinition();
    const TArray<TSubclassOf<UGameplayAbility>>* AbilitiesToGrant = &DefaultAbilities;
    if (ResolvedDefinition && !ResolvedDefinition->AbilitySet.IsNull())
    {
        if (ULastFPSAbilitySet* AbilitySet = ResolvedDefinition->AbilitySet.LoadSynchronous())
        {
            AbilitiesToGrant = &AbilitySet->GrantedAbilities;
        }
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : *AbilitiesToGrant)
    {
        if (AbilityClass && !ASC->FindAbilitySpecFromClass(AbilityClass))
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
    if (ResolvedDefinition && !ResolvedDefinition->AbilitySet.IsNull())
    {
        if (ULastFPSAbilitySet* AbilitySet = ResolvedDefinition->AbilitySet.LoadSynchronous())
        {
            EffectsToApply = &AbilitySet->StartupEffects;
        }
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

void ALastFPSCharacterBase::OnRep_CharacterDefinition()
{
    if (CharacterDefinition)
    {
        ApplyCharacterVisuals(CharacterDefinition);
    }
}
