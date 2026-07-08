#include "Economy/LastFPSItemPickupActor.h"

#include "Character/LastFPSHero.h"
#include "Data/Tables/LastFPSItemData.h"
#include "Economy/LastFPSEconomySubsystem.h"
#include "Game/LastFPSPlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSPickup, Log, All);

ALastFPSItemPickupActor::ALastFPSItemPickupActor()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // 발사 연출 중에만 켠다.

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetSphereRadius(PickupRadius);
    OverlapSphere->SetCollisionProfileName(TEXT("Trigger"));
    OverlapSphere->SetGenerateOverlapEvents(true);
    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ALastFPSItemPickupActor::OnOverlapBegin);
    RootComponent = OverlapSphere;

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
    PickupMesh->SetupAttachment(RootComponent);

    // 메시만 로컬 회전 — 트리거 스피어(루트)는 고정 유지. 로컬 틱이라 복제 불필요. (RotationRate 는 BeginPlay 에서 적용)
    RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
    RotatingMovement->SetUpdatedComponent(PickupMesh);
}

void ALastFPSItemPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALastFPSItemPickupActor, ItemRowId);
    DOREPLIFETIME(ALastFPSItemPickupActor, LaunchStartOffset);
}

void ALastFPSItemPickupActor::OnRep_ItemRowId()
{
    ApplyRarityVisual();
}

void ALastFPSItemPickupActor::ApplyRarityVisual()
{
    if (!PickupMesh || ItemRowId.IsNone())
    {
        return;
    }

    ELastFPSItemRarity Rarity = ELastFPSItemRarity::Common;
    bool bFoundRarity = false;
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const ULastFPSEconomySubsystem* Economy = GI->GetSubsystem<ULastFPSEconomySubsystem>())
        {
            bFoundRarity = Economy->TryGetItemRarity(ItemRowId, Rarity); // 실패 시 Common 유지
        }
    }

    FLinearColor Color = LastFPSGetRarityColor(Rarity) * EmissiveIntensity;
    Color.A = 1.f;

    // 어느 슬롯에 발광 머티리얼이 있는지 모르므로 전 슬롯의 MID 에 파라미터를 세팅(없는 슬롯은 무시됨).
    const int32 NumMaterials = PickupMesh->GetNumMaterials();
    bool bParamFoundAnySlot = false;
    for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
    {
        if (UMaterialInstanceDynamic* MID = PickupMesh->CreateAndSetMaterialInstanceDynamic(SlotIndex))
        {
            FLinearColor ExistingValue;
            if (MID->GetVectorParameterValue(FMaterialParameterInfo(EmissiveParameterName), ExistingValue))
            {
                bParamFoundAnySlot = true;
            }
            MID->SetVectorParameterValue(EmissiveParameterName, Color);
        }
    }

    // [진단] 원인 확정용 임시 로그 — 문제 해결 후 제거.
    UE_LOG(LogLastFPSPickup, Warning,
        TEXT("[Pickup] RowId=%s 등급조회=%s Rarity=%d Color=%s 머티리얼슬롯=%d 파라미터'%s'존재=%s"),
        *ItemRowId.ToString(),
        bFoundRarity ? TEXT("성공") : TEXT("실패(→Common)"),
        (int32)Rarity,
        *Color.ToString(),
        NumMaterials,
        *EmissiveParameterName.ToString(),
        bParamFoundAnySlot ? TEXT("YES") : TEXT("NO(이름불일치?)"));
}

void ALastFPSItemPickupActor::BeginPlay()
{
    Super::BeginPlay();

    // 서버/스탠드얼론: 스폰 시 RowId 가 이미 세팅됨. 순수 클라: OnRep_ItemRowId 가 도착 시 재적용.
    ApplyRarityVisual();

    // 에디터에서 조정한 RotationRate 반영 (생성자 이후 값이 세팅되므로 여기서 적용).
    if (RotatingMovement)
    {
        RotatingMovement->RotationRate = RotationRate;
    }

    // 발사 연출 준비(전 인스턴스). 오프셋이 있으면 날아가는 동안 픽업 불가.
    TryStartLaunch();

    if (!HasAuthority())
    {
        return;
    }

    // 연출이 없으면 즉시 착지 처리. 연출 중이면 서버 Tick 이 착지 시 HandleLanded 호출.
    if (!bLaunching)
    {
        HandleLanded();
    }
}

void ALastFPSItemPickupActor::TryStartLaunch()
{
    if (bLaunchResolved)
    {
        return;
    }

    if (LaunchStartOffset.IsNearlyZero() || LaunchDuration <= 0.f || !PickupMesh)
    {
        bLaunchResolved = true; // 연출 없음 — 착지 상태는 BeginPlay(서버)에서 결정.
        return;
    }

    bLaunchResolved = true;
    bLaunching = true;
    LaunchElapsed = 0.f;
    MeshRestRelativeLocation = PickupMesh->GetRelativeLocation(); // 변위 전 원래 위치 캐시.
    PickupMesh->SetRelativeLocation(MeshRestRelativeLocation + LaunchStartOffset);
    SetActorTickEnabled(true);
}

void ALastFPSItemPickupActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bLaunching)
    {
        return;
    }

    LaunchElapsed += DeltaSeconds;
    const float Alpha = (LaunchDuration > 0.f) ? FMath::Clamp(LaunchElapsed / LaunchDuration, 0.f, 1.f) : 1.f;

    // 수평/기저 위치는 시작 오프셋→0 으로 보간, 수직은 4h·a(1-a) 포물선(양 끝 0, 중앙 정점 h).
    const FVector Base = FMath::Lerp(LaunchStartOffset, FVector::ZeroVector, Alpha);
    const float ArcZ = 4.f * LaunchArcHeight * Alpha * (1.f - Alpha);
    if (PickupMesh)
    {
        PickupMesh->SetRelativeLocation(MeshRestRelativeLocation + Base + FVector(0.f, 0.f, ArcZ));
    }

    if (Alpha >= 1.f)
    {
        bLaunching = false;
        if (PickupMesh)
        {
            PickupMesh->SetRelativeLocation(MeshRestRelativeLocation);
        }
        SetActorTickEnabled(false);

        if (HasAuthority())
        {
            HandleLanded();
        }
    }
}

void ALastFPSItemPickupActor::HandleLanded()
{
    if (bLanded)
    {
        return;
    }
    bLanded = true;

    OverlapSphere->SetSphereRadius(PickupRadius);

    // 착지 순간 이미 겹쳐 있던 Hero 즉시 처리.
    TArray<AActor*> Overlapping;
    OverlapSphere->GetOverlappingActors(Overlapping, ALastFPSHero::StaticClass());
    for (AActor* Actor : Overlapping)
    {
        TryGrant(Actor);
        if (IsActorBeingDestroyed())
        {
            break;
        }
    }
}

void ALastFPSItemPickupActor::OnRep_LaunchOffset()
{
    // 순수 클라: 오프셋이 늦게 도착하면 여기서 연출 시작(메시 기준 위치는 BeginPlay 에서 이미 캐시).
    TryStartLaunch();
}

void ALastFPSItemPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                             bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    TryGrant(OtherActor);
}

void ALastFPSItemPickupActor::TryGrant(AActor* OtherActor)
{
    // 아직 날아가는 중(착지 전)이면 줍지 못한다.
    if (!bLanded)
    {
        return;
    }

    ALastFPSHero* Hero = Cast<ALastFPSHero>(OtherActor);
    if (!Hero)
    {
        return;
    }

    // 지급은 주운 플레이어의 PlayerState 로 위임 — 그 플레이어 소유 클라의 로컬 Economy 에 들어간다.
    if (ALastFPSPlayerState* PS = Hero->GetPlayerState<ALastFPSPlayerState>())
    {
        PS->Auth_GrantItem(ItemRowId, Count);
    }

    Destroy();
}
