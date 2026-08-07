#include "Character/LastFPSEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/AI/LastFPSEnemyAIController.h"
#include "Character/LastFPSAIProfile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/Components/LastFPSCombatAimComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Data/Definitions/LastFPSEnemyDefinition.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Engine/World.h"
#include "Game/LastFPSGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Perception/AIPerceptionComponent.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEnemyCharacter, Log, All);

ALastFPSEnemyCharacter::ALastFPSEnemyCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    // 기본 두뇌를 전투 컨트롤러로. BP 에서 개별 오버라이드 가능.
    AIControllerClass = ALastFPSEnemyAIController::StaticClass();

    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComp");
    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
    CombatAimComponent = CreateDefaultSubobject<ULastFPSCombatAimComponent>(TEXT("CombatAimComp"));

    // AI 는 컨트롤러가 SetFocus 로 준 방향(desired rotation)으로 몸을 돌린다.
    // 플레이어(Hero)와 달리 컨트롤러 Yaw 를 직접 쓰지 않고 이동 컴포넌트가 부드럽게 회전시킨다.
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll  = false;
    bUseControllerRotationYaw   = false;
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->bUseControllerDesiredRotation = true;
        Movement->bOrientRotationToMovement     = false;
        Movement->RotationRate = FRotator(0.f, 360.f, 0.f);
    }

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
}

const ULastFPSAIProfile* ALastFPSEnemyCharacter::GetAIProfile() const
{
    if (const ULastFPSEnemyDefinition* EnemyDef = Cast<ULastFPSEnemyDefinition>(ResolveCharacterDefinition()))
    {
        return EnemyDef->AIProfile;
    }
    return nullptr;
}

void ALastFPSEnemyCharacter::PostInitializeComponents()
{
    // APawn이 자동 AI Controller를 생성하기 전에 프로필의 Controller 클래스를 반영해야 한다.
    ApplyAIControllerClassFromProfile();
    Super::PostInitializeComponents();
}

void ALastFPSEnemyCharacter::ApplyAIControllerClassFromProfile()
{
    const ULastFPSAIProfile* Profile = GetAIProfile();
    if (Profile && Profile->AIControllerClass)
    {
        AIControllerClass = Profile->AIControllerClass;
    }
}

void ALastFPSEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (USkeletalMeshComponent* MeshComp = GetMesh();
        MeshComp && !bMeshRelativeTransformCaptured)
    {
        InitialMeshRelativeTransform = MeshComp->GetRelativeTransform();
        bMeshRelativeTransformCaptured = true;
    }

    if (HasAuthority())
    {
        OnDeath.AddUObject(this, &ALastFPSEnemyCharacter::HandleOwnDeath);

        const ULastFPSEnemyDefinition* EnemyDefinition =
            Cast<ULastFPSEnemyDefinition>(ResolveCharacterDefinition());
        if (WeaponComponent && EnemyDefinition && EnemyDefinition->InitialWeaponDefinition)
        {
            WeaponComponent->EquipWeaponDefinition(EnemyDefinition->InitialWeaponDefinition);
        }
    }
}

void ALastFPSEnemyCharacter::HandleOwnDeath(ALastFPSCharacterBase* /*DeadChar*/)
{
    Multicast_ApplyDeathRagdollImpulse(GetLastDamageImpulseDirection());
    GetWorldTimerManager().ClearTimer(DeathRemovalTimerHandle);
    GetWorldTimerManager().SetTimer(
        DeathRemovalTimerHandle,
        this,
        &ThisClass::FinishDeathRemoval,
        FMath::Max(DeathRemovalDelay, 0.1f),
        false);

    if (const UWorld* World = GetWorld())
    {
        if (const ALastFPSGameModeBase* GameMode = World->GetAuthGameMode<ALastFPSGameModeBase>())
        {
            GameMode->SpawnDropsForDeath(*this);
        }
    }
}

void ALastFPSEnemyCharacter::UpdateAliveCollisionState(bool bAlive)
{
    // 서버에서 무기 액터를 먼저 제거해 랙돌 전환 후 무기가 시체에 남지 않게 한다.
    if (!bAlive && HasAuthority() && WeaponComponent && WeaponComponent->HasWeapon())
    {
        WeaponComponent->UnequipWeapon();
    }

    if (bAlive)
    {
        ResetDeathRagdoll();
    }

    Super::UpdateAliveCollisionState(bAlive);

    if (!bAlive)
    {
        StartDeathRagdoll();
    }
}

void ALastFPSEnemyCharacter::ResetDeathRagdoll()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        return;
    }

    MeshComp->SetAllBodiesPhysicsBlendWeight(0.f, false);
    MeshComp->SetAllBodiesSimulatePhysics(false);
    MeshComp->SetSimulatePhysics(false);
    MeshComp->SetEnableGravity(false);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComp->bPauseAnims = false;

    if (bMeshRelativeTransformCaptured)
    {
        MeshComp->SetRelativeTransform(
            InitialMeshRelativeTransform,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }
    bDeathRagdollStarted = false;
}

void ALastFPSEnemyCharacter::FinishDeathRemoval()
{
    if (ULastFPSActorPoolSubsystem* Pool =
        GetWorld() ? GetWorld()->GetSubsystem<ULastFPSActorPoolSubsystem>() : nullptr)
    {
        if (Pool->ReleaseActor(this))
        {
            return;
        }
    }
    Destroy();
}

void ALastFPSEnemyCharacter::ResetForPoolReuse(
    ULastFPSCharacterDefinition* InDefinition)
{
    GetWorldTimerManager().ClearTimer(DeathRemovalTimerHandle);
    ResetDeathRagdoll();
    Super::ResetForPoolReuse(InDefinition);

    if (!HasActorBegunPlay())
    {
        return;
    }

    ApplyAIControllerClassFromProfile();
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->Activate(true);
    }

    const ULastFPSEnemyDefinition* EnemyDefinition =
        Cast<ULastFPSEnemyDefinition>(ResolveCharacterDefinition());
    if (HasAuthority()
        && WeaponComponent
        && EnemyDefinition
        && EnemyDefinition->InitialWeaponDefinition
        && !WeaponComponent->HasWeapon())
    {
        WeaponComponent->EquipWeaponDefinition(
            EnemyDefinition->InitialWeaponDefinition);
    }
}

void ALastFPSEnemyCharacter::OnAcquiredFromPool_Implementation()
{
    GetWorldTimerManager().ClearTimer(DeathRemovalTimerHandle);
    ResetDeathRagdoll();
    SetActorEnableCollision(true);
}

void ALastFPSEnemyCharacter::OnReleasedToPool_Implementation()
{
    GetWorldTimerManager().ClearTimer(DeathRemovalTimerHandle);
    ResetDeathRagdoll();
    ClearRecentAttackers();

    if (HasAuthority())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->CancelAllAbilities();
            ASC->RemoveActiveEffects(FGameplayEffectQuery());
        }
    }

    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->Deactivate();
    }
    if (HasAuthority() && WeaponComponent && WeaponComponent->HasWeapon())
    {
        WeaponComponent->UnequipWeapon();
    }
}

void ALastFPSEnemyCharacter::OnPrepareForPoolRenderWarmup_Implementation()
{
    ResetDeathRagdoll();
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(true, true);
    }
}

void ALastFPSEnemyCharacter::StartDeathRagdoll()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp || !MeshComp->GetPhysicsAsset())
    {
        UE_LOG(LogLastFPSEnemyCharacter, Warning,
            TEXT("적 랙돌 전환 실패: Enemy=%s, 원인=유효한 PhysicsAsset이 없습니다."),
            *GetNameSafe(this));
        return;
    }

    // 체력 복제나 사망 RPC 순서에 따라 이 함수가 여러 번 호출될 수 있다.
    // 이전 호출 이후 충돌 상태가 덮어써졌더라도 랙돌에 필요한 상태는 항상 복구한다.
    // 다른 애니메이션 시스템의 정지 상태까지 포함해 물리 전환 전에는 반드시 포즈 고정을 해제한다.
    MeshComp->bPauseAnims = false;
    MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    MeshComp->SetEnableGravity(true);
    MeshComp->SetEnableGravityOnAllBodiesBelow(true, NAME_None, /*bIncludeSelf*/ true);
    MeshComp->SetSimulatePhysics(true);
    MeshComp->SetAllBodiesSimulatePhysics(true);
    MeshComp->SetAllBodiesPhysicsBlendWeight(1.f, /*bSkipCustomPhysicsType*/ false);
    MeshComp->WakeAllRigidBodies();
    bDeathRagdollStarted = true;
}

void ALastFPSEnemyCharacter::Multicast_ApplyDeathRagdollImpulse_Implementation(
    const FVector_NetQuantizeNormal ImpulseDirection)
{
    StartDeathRagdoll();

    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp || !bDeathRagdollStarted)
    {
        return;
    }

    const FVector Impulse =
        FVector(ImpulseDirection) * FMath::Max(DeathRagdollImpulseVelocity, 0.f)
        + FVector::UpVector * FMath::Max(DeathRagdollUpwardVelocity, 0.f);
    if (Impulse.IsNearlyZero())
    {
        return;
    }

    MeshComp->AddImpulseToAllBodiesBelow(
        Impulse,
        DeathRagdollImpulseBoneName,
        /*bVelChange*/ true,
        /*bIncludeSelf*/ true);
}

void ALastFPSEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
}
