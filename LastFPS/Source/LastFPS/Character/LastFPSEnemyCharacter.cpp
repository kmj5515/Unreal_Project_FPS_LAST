#include "Character/LastFPSEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/LastFPSAttributeSet.h"
#include "Character/AI/LastFPSEnemyAIController.h"
#include "Character/LastFPSAIProfile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/Components/LastFPSCombatAimComponent.h"
#include "Character/Components/WeaponComponent.h"
#include "Data/Definitions/LastFPSEnemyDefinition.h"
#include "Data/Definitions/LastFPSWeaponDefinition.h"
#include "Economy/LastFPSItemPickupActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Perception/AIPerceptionComponent.h"
#include "Pooling/LastFPSActorPoolSpawn.h"
#include "Pooling/LastFPSActorPoolSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "UI/HUD/LastFPSWaveEnemyMarkerWidget.h"
#include "UI/Theme/LastFPSUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSEnemyCharacter, Log, All);

ALastFPSEnemyCharacter::ALastFPSEnemyCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    // 기본 두뇌를 전투 컨트롤러로. BP 에서 개별 오버라이드 가능.
    AIControllerClass = ALastFPSEnemyAIController::StaticClass();

    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComp");
    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
    CombatAimComponent = CreateDefaultSubobject<ULastFPSCombatAimComponent>(TEXT("CombatAimComp"));

    WaveEnemyMarkerComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WaveEnemyMarker"));
    WaveEnemyMarkerComponent->SetupAttachment(GetRootComponent());
    WaveEnemyMarkerComponent->SetWidgetClass(ULastFPSWaveEnemyMarkerWidget::StaticClass());
    WaveEnemyMarkerComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WaveEnemyMarkerComponent->SetDrawSize(FVector2D(64.f, 64.f));
    WaveEnemyMarkerComponent->SetPivot(FVector2D(0.5f, 1.f));
    WaveEnemyMarkerComponent->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
    WaveEnemyMarkerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaveEnemyMarkerComponent->SetVisibility(false);

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

void ALastFPSEnemyCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSEnemyCharacter, bWaveEnemyMarkerVisible);
}

void ALastFPSEnemyCharacter::SetWaveEnemyMarkerVisible(const bool bVisible)
{
    if (!HasAuthority() || bWaveEnemyMarkerVisible == bVisible)
    {
        return;
    }

    bWaveEnemyMarkerVisible = bVisible;
    ApplyWaveEnemyMarkerVisibility();
    ForceNetUpdate();
}

void ALastFPSEnemyCharacter::OnRep_WaveEnemyMarkerVisible()
{
    ApplyWaveEnemyMarkerVisibility();
}

void ALastFPSEnemyCharacter::ApplyWaveEnemyMarkerVisibility()
{
    if (WaveEnemyMarkerComponent)
    {
        WaveEnemyMarkerComponent->SetVisibility(bWaveEnemyMarkerVisible, true);
    }
}

const ULastFPSAIProfile* ALastFPSEnemyCharacter::GetAIProfile() const
{
    if (const ULastFPSEnemyDefinition* EnemyDef = Cast<ULastFPSEnemyDefinition>(ResolveCharacterDefinition()))
    {
        return EnemyDef->AIProfile.LoadSynchronous();
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

    if (WaveEnemyMarkerComponent)
    {
        const ULastFPSUISettings* UISettings = ULastFPSUISettings::Get();
        if (UISettings && !UISettings->WaveEnemyMarkerWidgetClass.IsNull())
        {
            if (UClass* MarkerWidgetClass = UISettings->WaveEnemyMarkerWidgetClass.LoadSynchronous())
            {
                WaveEnemyMarkerComponent->SetWidgetClass(MarkerWidgetClass);
                WaveEnemyMarkerComponent->InitWidget();
            }
            else
            {
                static bool bLoggedMissingWaveMarkerClass = false;
                if (!bLoggedMissingWaveMarkerClass)
                {
                    bLoggedMissingWaveMarkerClass = true;
                    UE_LOG(
                        LogLastFPSEnemyCharacter,
                        Error,
                        TEXT("'%s': 웨이브 적 마커 위젯을 불러오지 못했습니다. 설정 경로: '%s'"),
                        *GetName(),
                        *UISettings->WaveEnemyMarkerWidgetClass.ToSoftObjectPath().ToString());
                }
            }
        }

        if (GetCapsuleComponent())
        {
            WaveEnemyMarkerComponent->SetRelativeLocation(FVector(
                0.f,
                0.f,
                GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + WaveEnemyMarkerVerticalOffset));
        }
    }
    ApplyWaveEnemyMarkerVisibility();

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
        if (WeaponComponent && EnemyDefinition && !EnemyDefinition->InitialWeaponDefinition.IsNull())
        {
            WeaponComponent->EquipWeaponDefinition(EnemyDefinition->InitialWeaponDefinition.LoadSynchronous());
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

    UWorld* World = GetWorld();
    if (!DropPickupClass || !World)
    {
        return;
    }

    // 최종 스폰할 RowId 목록을 구성: (1) 확정분은 항상, (2) 남은 개수만 가중치 랜덤으로 채움.
    TArray<FName> SpawnRowIds;
    for (const FName& RowId : GuaranteedDropRowIds)
    {
        if (!RowId.IsNone())
        {
            SpawnRowIds.Add(RowId);
        }
    }

    const int32 RandomCount = FMath::Max(0, DropCount - SpawnRowIds.Num());
    if (RandomCount > 0)
    {
        // 유효(가중치>0) 후보의 가중치 합. 0이면 랜덤분은 건너뛴다(확정분은 그대로 나감).
        float TotalWeight = 0.f;
        for (const FLastFPSEnemyDropEntry& Entry : DropTable)
        {
            if (!Entry.ItemRowId.IsNone() && Entry.Weight > 0.f)
            {
                TotalWeight += Entry.Weight;
            }
        }
        if (TotalWeight > 0.f)
        {
            for (int32 i = 0; i < RandomCount; ++i)
            {
                const FName RowId = PickWeightedDropRowId(TotalWeight);
                if (!RowId.IsNone())
                {
                    SpawnRowIds.Add(RowId);
                }
            }
        }
    }

    const int32 Total = SpawnRowIds.Num();
    if (Total <= 0)
    {
        return;
    }

    // 사망 위치를 중심으로 원주에 균등 간격으로 동시 스폰.
    const FVector Center = GetActorLocation();
    for (int32 i = 0; i < Total; ++i)
    {
        const float AngleRad = (2.f * PI * i) / Total;
        const FVector Offset(FMath::Cos(AngleRad) * DropSpreadRadius,
                             FMath::Sin(AngleRad) * DropSpreadRadius, 0.f);
        const FTransform SpawnTransform(FRotator::ZeroRotator, Center + Offset);

        // 착지 지점(링)에서 적 중심으로의 오프셋 = 발사 시작점. 중심에서 튀어나와 포물선으로 링에 안착.
        const FVector LaunchOffset = Center - SpawnTransform.GetLocation();
        LastFPSActorPool::AcquireOrSpawnDeferred<ALastFPSItemPickupActor>(
            *World,
            DropPickupClass,
            SpawnTransform,
            this,
            nullptr,
            [RowId = SpawnRowIds[i], LaunchOffset](ALastFPSItemPickupActor& Pickup)
            {
                Pickup.InitializePickup(RowId, 1, LaunchOffset);
            });
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
    if (HasAuthority())
    {
        SetWaveEnemyMarkerVisible(false);
    }
    else
    {
        ApplyWaveEnemyMarkerVisibility();
    }

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
        && !EnemyDefinition->InitialWeaponDefinition.IsNull()
        && !WeaponComponent->HasWeapon())
    {
        WeaponComponent->EquipWeaponDefinition(
            EnemyDefinition->InitialWeaponDefinition.LoadSynchronous());
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
        SetWaveEnemyMarkerVisible(false);
    }
    else if (WaveEnemyMarkerComponent)
    {
        WaveEnemyMarkerComponent->SetVisibility(false, true);
    }

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

FName ALastFPSEnemyCharacter::PickWeightedDropRowId(float TotalWeight) const
{
    float Roll = FMath::FRandRange(0.f, TotalWeight);
    FName LastValid = NAME_None; // 부동소수 오차로 루프를 빠져나가는 경우의 폴백.
    for (const FLastFPSEnemyDropEntry& Entry : DropTable)
    {
        if (Entry.ItemRowId.IsNone() || Entry.Weight <= 0.f)
        {
            continue;
        }
        LastValid = Entry.ItemRowId;
        Roll -= Entry.Weight;
        if (Roll <= 0.f)
        {
            return Entry.ItemRowId;
        }
    }
    return LastValid;
}

void ALastFPSEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
}
