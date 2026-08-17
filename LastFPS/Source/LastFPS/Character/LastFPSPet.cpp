#include "Character/LastFPSPet.h"
#include "Economy/LastFPSItemPickupActor.h"
#include "Character/LastFPSHero.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPet, Log, All);

ALastFPSPet::ALastFPSPet()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    // AIController가 빙의해도 이동 방향으로 회전하도록 컨트롤러 회전 비활성화
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // 캡슐을 강아지 크기에 맞게 줄인다
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCapsuleHalfHeight(35.f);
        Capsule->SetCapsuleRadius(25.f);
    }

    // CharacterMovement 기본 설정 — 바닥을 걸어다니도록
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->MaxWalkSpeed = 800.f;
        CMC->MaxAcceleration = 2000.f;
        CMC->BrakingDecelerationWalking = 2000.f;
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.f, 720.f, 0.f);
        CMC->GravityScale = 1.f;
        CMC->bConstrainToPlane = false;
    }

    // 메쉬 오프셋 (캡슐 바닥에 발이 닿도록, 정면을 보도록 회전)
    if (GetMesh())
    {
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -35.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetupAttachment(RootComponent);
    OverlapSphere->SetSphereRadius(PickupRadius);
    // 아이템(Pickup)은 ECC_Pawn 타입하고만 겹치도록 설정되어 있으므로, 펫의 픽업 반경도 Pawn으로 설정.
    // 길찾기나 총알 등에 영향을 주지 않도록 다른 모든 채널은 무시.
    OverlapSphere->SetCollisionObjectType(ECC_Pawn);
    OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    OverlapSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);

    DetectSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectSphere"));
    DetectSphere->SetupAttachment(RootComponent);
    DetectSphere->SetSphereRadius(DetectRadius);
    // 큰 감지 구체가 아이템 획득 Overlap까지 발생시키지 않도록 명시적 범위 쿼리만 사용한다.
    DetectSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DetectSphere->SetGenerateOverlapEvents(false);
}

void ALastFPSPet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSPet, PetState);
}

void ALastFPSPet::BeginPlay()
{
    Super::BeginPlay();

    if (DetectSphere)
    {
        DetectSphere->SetSphereRadius(DetectRadius);
    }
    if (OverlapSphere)
    {
        OverlapSphere->SetSphereRadius(PickupRadius);
    }

    // 기존 블루프린트에 저장된 값을 강제로 덮어쓰기 위해 여기서 한 번 더 설정
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->bOrientRotationToMovement = true;
        // 회전 속도를 720에서 360으로 줄여 조금 더 부드럽게 회전하도록 조정
        CMC->RotationRate = FRotator(0.f, 360.f, 0.f);
    }

    UE_LOG(LogLastFPSPet, Log, TEXT("[Pet] BeginPlay — Owner=%s, HasAuth=%d"),
        *GetNameSafe(GetOwner()), HasAuthority());
}

void ALastFPSPet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority())
    {
        UpdateMovement(DeltaTime);
    }
}

void ALastFPSPet::OnRep_PetState()
{
    // 애니메이션 블루프린트는 PetState를 읽어 활용
}

void ALastFPSPet::UpdateMovement(float DeltaTime)
{
    ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerHero.Get());

    // 워프 거리 체크 (주인 기준)
    if (AActor* HeroActor = OwnerHero.Get())
    {
        float DistToHero = FVector::Dist(HeroActor->GetActorLocation(), GetActorLocation());
        if (DistToHero > WarpDistance)
        {
            FVector HeroForward = HeroActor->GetActorForwardVector();
            FVector HeroRight = HeroActor->GetActorRightVector();
            FVector WarpLoc = HeroActor->GetActorLocation() 
                + (HeroForward * FollowOffset.X)
                + (HeroRight * FollowOffset.Y)
                + FVector(0.f, 0.f, FollowOffset.Z);
            
            // 순간이동임을 CMC와 클라이언트 스무딩에 알려야 한다.
            // SetActorLocation은 bJustTeleported를 세우지 않아 시뮬레이티드 프록시가
            // 워프 거리를 보간해버린다(펫이 맵을 가로질러 미끄러짐).
            TeleportTo(WarpLoc, GetActorRotation());

            if (AAIController* AICon = Cast<AAIController>(GetController()))
            {
                AICon->StopMovement();
                LastMoveTarget = FVector::ZeroVector;
            }
            
            // 타겟 무시하고 주인의 곁으로 즉시 복귀
            TargetItem = nullptr;
            return;
        }
    }

    // 타겟 유효성 검사 (액터 파괴, 풀(Pool) 반환, 그리고 주인에게 더는 지급되지 않는 픽업)
    if (!TargetItem.IsValid() || TargetItem->IsActorBeingDestroyed()
        || !Hero || !TargetItem->CanGrantTo(*Hero))
    {
        TargetItem = nullptr;

        // 아이템 스캔은 매 틱 하지 않고 0.5초마다 수행하여 최적화 및 타이밍 이슈 해결
        const float CurrentTime = GetWorld()->GetTimeSeconds();
        const float ScanInterval = FMath::Max(TargetScanInterval, 0.1f);
        if (LastTargetScanTime < 0.f || CurrentTime - LastTargetScanTime >= ScanInterval)
        {
            FindNextTarget();
            LastTargetScanTime = CurrentTime;
        }
    }

    FVector TargetLocation;
    float AcceptanceRadius = 50.f;

    if (TargetItem.IsValid())
    {
        TargetLocation = TargetItem->GetActorLocation();
    }
    else if (AActor* HeroActor = OwnerHero.Get())
    {
        FVector OwnerLoc = HeroActor->GetActorLocation();
        FVector OwnerForward = HeroActor->GetActorForwardVector();
        FVector OwnerRight = HeroActor->GetActorRightVector();

        // 주인 왼쪽 뒤
        TargetLocation = OwnerLoc
            + (OwnerForward * FollowOffset.X)
            + (OwnerRight * FollowOffset.Y);

        AcceptanceRadius = 150.f;
    }
    else
    {
        return;
    }

    // 수평 거리만 비교 (높이 차이로 인한 오판 방지)
    FVector ToTarget = TargetLocation - GetActorLocation();
    ToTarget.Z = 0.f;
    float Distance = ToTarget.Size();

    if (Distance > AcceptanceRadius)
    {
        // AIController 길찾기 이동 — 목표가 크게 변했거나 길찾기가 멈췄을 때 재발행
        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            bool bNeedsNewMove = false;
            if (FVector::Dist(LastMoveTarget, TargetLocation) > 50.f)
            {
                bNeedsNewMove = true;
            }
            else if (UPathFollowingComponent* PathComp = AICon->GetPathFollowingComponent())
            {
                // 타겟은 그대로지만 길찾기에 실패했거나 멈췄을 경우 재시도
                if (PathComp->GetStatus() == EPathFollowingStatus::Idle)
                {
                    bNeedsNewMove = true;
                }
            }

            float CurrentTime = GetWorld()->GetTimeSeconds();
            // 재시도 스팸을 막기 위해 0.5초 쿨타임 적용
            if (bNeedsNewMove && (CurrentTime - LastMoveRequestTime > 0.5f))
            {
                const bool bMovingToItem = TargetItem.IsValid();
                const EPathFollowingRequestResult::Type MoveResult = AICon->MoveToLocation(
                    TargetLocation, AcceptanceRadius * 0.5f,
                    /*bStopOnOverlap=*/ true,
                    /*bUsePathfinding=*/ true,
                    /*bProjectDestinationToNavigation=*/ true,
                    /*bCanStrafe=*/ false,
                    /*필터 클래스=*/ nullptr,
                    /*부분 경로 허용=*/ false);
                LastMoveRequestTime = CurrentTime;

                if (MoveResult == EPathFollowingRequestResult::Failed)
                {
                    LastMoveTarget = FVector::ZeroVector;
                    if (bMovingToItem)
                    {
                        TargetItem.Reset();
                        LastTargetScanTime = CurrentTime;
                    }
                }
                else
                {
                    LastMoveTarget = TargetLocation;
                }
            }
        }
        else
        {
            // 컨트롤러가 없으면 직접 이동
            FVector MoveDir = ToTarget.GetSafeNormal();
            AddMovementInput(MoveDir, 1.f);
        }

        // 이동 중 상태 갱신
        ELastFPSPetState NewState = TargetItem.IsValid() ? ELastFPSPetState::Looting : ELastFPSPetState::Following;
        if (PetState != NewState)
        {
            PetState = NewState;
        }

        // 회전은 CMC의 bOrientRotationToMovement에 맡긴다.
        // 서버에서 매 틱 SetActorRotation으로 덮으면 양자화된 복제 회전과
        // 클라이언트 로컬 회전이 서로 싸워 떨림이 생긴다. 속도 조절은 RotationRate로 한다.
    }
    else
    {
        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            AICon->StopMovement();
            LastMoveTarget = FVector::ZeroVector;
        }

        if (PetState != ELastFPSPetState::Idle)
        {
            PetState = ELastFPSPetState::Idle;
        }
    }
}

void ALastFPSPet::FindNextTarget()
{
    TargetItem = nullptr;

    // 주인이 없으면 지급 대상이 없으므로 탐색 자체가 의미 없다.
    const ALastFPSHero* Hero = Cast<ALastFPSHero>(OwnerHero.Get());
    if (!Hero)
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectParams;
    // PickupObjectChannel (ECC_GameTraceChannel1) 만 검출
    ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);

    FCollisionShape SphereShape = FCollisionShape::MakeSphere(DetectRadius);
    GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, SphereShape);

    const FVector PetLocation = GetActorLocation();
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (const FOverlapResult& Result : Overlaps)
    {
        if (ALastFPSItemPickupActor* Item = Cast<ALastFPSItemPickupActor>(Result.GetActor()))
        {
            if (Item->IsActorBeingDestroyed() || !Item->CanGrantTo(*Hero))
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(PetLocation, Item->GetActorLocation());
            if (DistanceSquared >= BestDistanceSquared || !IsItemReachable(*Item))
            {
                continue;
            }

            TargetItem = Item;
            BestDistanceSquared = DistanceSquared;
        }
    }
}

bool ALastFPSPet::IsItemReachable(const ALastFPSItemPickupActor& Item) const
{
    if (!Cast<AAIController>(GetController()))
    {
        return true;
    }

    UWorld* World = GetWorld();
    UNavigationSystemV1* NavigationSystem = World
        ? UNavigationSystemV1::GetCurrent(World)
        : nullptr;
    if (!NavigationSystem)
    {
        return false;
    }

    FNavLocation ProjectedLocation;
    const FNavAgentProperties& AgentProperties = GetNavAgentPropertiesRef();
    if (!NavigationSystem->ProjectPointToNavigation(
            Item.GetActorLocation(), ProjectedLocation, INVALID_NAVEXTENT, &AgentProperties))
    {
        return false;
    }

    const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        World,
        GetActorLocation(),
        ProjectedLocation.Location,
        const_cast<ALastFPSPet*>(this));
    return IsValid(Path) && Path->IsValid() && !Path->IsPartial();
}
