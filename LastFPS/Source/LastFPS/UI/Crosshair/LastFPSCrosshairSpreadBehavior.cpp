#include "UI/Crosshair/LastFPSCrosshairSpreadBehavior.h"

#include "Character/LastFPSCharacterBase.h"

#include "EasyCrosshairSystem/ecsCrosshairEditorAsset.h"
#include "EasyCrosshairSystem/ecsCrosshairWidget.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LastFPSCrosshairSpreadBehavior)

DEFINE_LOG_CATEGORY_STATIC(LogLastFPSCrosshair, Log, All);

ULastFPSCrosshairSpreadBehavior::ULastFPSCrosshairSpreadBehavior()
{
    // 십자형 4레이어 기본값. 에셋의 레이어 키가 다르면 에셋 쪽 인스턴스에서 재설정한다.
    SpreadLayerDirections.Add(TEXT("Top"), FVector2D(0.f, -1.f));
    SpreadLayerDirections.Add(TEXT("Bottom"), FVector2D(0.f, 1.f));
    SpreadLayerDirections.Add(TEXT("Left"), FVector2D(-1.f, 0.f));
    SpreadLayerDirections.Add(TEXT("Right"), FVector2D(1.f, 0.f));
}

void ULastFPSCrosshairSpreadBehavior::BeginBehavior()
{
    Super::BeginBehavior();

    CachedLayerBases.Reset();
    LastTickSeconds = FPlatformTime::Seconds();
    CurrentSpreadPx = BaseSpreadPx;
    LastAppliedSpreadPx = -1.f;
    FireSpreadPx = 0.f;

    const UecsCrosshairEditorAsset* Asset = CrosshairWidget ? CrosshairWidget->GetCrosshairEditorAsset() : nullptr;
    if (!Asset)
    {
        UE_LOG(LogLastFPSCrosshair, Warning,
            TEXT("%s: 크로스헤어 에셋을 찾을 수 없어 스프레드를 비활성화합니다."), *GetNameSafe(this));
        return;
    }

    // 절대값 교체 방식(SetLayerTransform) 보정을 위해 레이어 기본 변환을 1회 캐시.
    for (const TPair<FName, FVector2D>& Pair : SpreadLayerDirections)
    {
        const FecsCrosshairLayerData* Layer = Asset->Layers.Find(Pair.Key.ToString());
        if (!Layer || !Layer->IsValid())
        {
            // 프레임 반복 경고를 피하기 위해 여기서 1회만 알린다.
            UE_LOG(LogLastFPSCrosshair, Warning,
                TEXT("%s: 에셋 '%s'에 레이어 '%s'가 없어 해당 레이어는 스프레드에서 제외됩니다."),
                *GetNameSafe(this), *GetNameSafe(Asset), *Pair.Key.ToString());
            continue;
        }

        FCachedLayerBase& Cached = CachedLayerBases.Add(Pair.Key);
        Cached.Direction = Pair.Value.GetSafeNormal();
        Cached.BaseTransform = Layer->Transform;
        Cached.BaseRotation = Layer->Rotation;
        Cached.BaseScale = Layer->Scale;
    }
}

void ULastFPSCrosshairSpreadBehavior::Tick()
{
    Super::Tick();

    if (CachedLayerBases.IsEmpty())
    {
        return;
    }

    // 플러그인 Tick은 DeltaTime을 주지 않고 TickRate 간격으로 호출되므로 자체 계측한다.
    const double NowSeconds = FPlatformTime::Seconds();
    const float DeltaSeconds = FMath::Clamp(static_cast<float>(NowSeconds - LastTickSeconds), 0.f, 0.1f);
    LastTickSeconds = NowSeconds;

    // 연사 누적분은 상태 스프레드와 별도로 감쇠 — ADS 배율의 영향을 받지 않고
    // 발사 직후 최대치에서 시간에 따라 0으로 복귀한다(기존 HUD 모델과 동일).
    FireSpreadPx = FMath::FInterpTo(FireSpreadPx, 0.f, DeltaSeconds, FireSpreadRecoverSpeed);

    const APawn* Pawn = ResolveOwningPawnSafe();
    const float TargetSpreadPx = (Pawn ? CalculateTargetSpreadPx(*Pawn) : BaseSpreadPx) + FireSpreadPx;

    CurrentSpreadPx = FMath::FInterpTo(CurrentSpreadPx, TargetSpreadPx, DeltaSeconds, InterpSpeed);

    // 변화가 없으면 Slate 변환 재계산을 생략한다.
    if (FMath::IsNearlyEqual(CurrentSpreadPx, LastAppliedSpreadPx, 0.01f))
    {
        return;
    }

    ApplySpreadToLayers(CurrentSpreadPx);
    LastAppliedSpreadPx = CurrentSpreadPx;
}

void ULastFPSCrosshairSpreadBehavior::NotifyWeaponFired()
{
    FireSpreadPx = FMath::Min(FireSpreadPx + FireSpreadAddPx, FireSpreadMaxPx);
}

float ULastFPSCrosshairSpreadBehavior::CalculateTargetSpreadPx(const APawn& Pawn) const
{
    float TargetSpreadPx = BaseSpreadPx;

    const FVector Velocity = Pawn.GetVelocity();
    if (FVector(Velocity.X, Velocity.Y, 0.f).Size() > MovementSpeedThreshold)
    {
        TargetSpreadPx += MoveSpreadPx;
    }

    if (const ACharacter* Character = Cast<ACharacter>(&Pawn))
    {
        if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
            Movement && Movement->IsFalling())
        {
            TargetSpreadPx += JumpSpreadPx;
        }
    }

    if (const ALastFPSCharacterBase* LastFPSCharacter = Cast<ALastFPSCharacterBase>(&Pawn);
        LastFPSCharacter && LastFPSCharacter->GetIsADS())
    {
        TargetSpreadPx *= AdsSpreadMultiplier;
    }

    return TargetSpreadPx;
}

void ULastFPSCrosshairSpreadBehavior::ApplySpreadToLayers(const float SpreadPx)
{
    for (const TPair<FName, FCachedLayerBase>& Pair : CachedLayerBases)
    {
        const FCachedLayerBase& Cached = Pair.Value;
        SetLayerTransform(
            Pair.Key,
            Cached.BaseTransform + Cached.Direction * SpreadPx,
            Cached.BaseRotation,
            Cached.BaseScale);
    }
}

APawn* ULastFPSCrosshairSpreadBehavior::ResolveOwningPawnSafe() const
{
    // 기본 GetOwningPawn은 OwningPlayer를 null 확인 없이 역참조하므로 사용하지 않는다.
    if (!CrosshairWidget)
    {
        return nullptr;
    }

    // ecs 위젯은 서브시스템이 CreateWidget(World)로 만든 뒤 HUD의 CrosshairHost로
    // 재부착되므로 OwningPlayer가 비어 있을 수 있다 → 첫 로컬 PlayerController로 폴백.
    // 크로스헤어는 로컬 단일 인스턴스 전제라 첫 로컬 플레이어가 곧 소유자다.
    const APlayerController* OwningPlayer = CrosshairWidget->GetOwningPlayer();
    if (!OwningPlayer)
    {
        const UWorld* World = CrosshairWidget->GetWorld();
        OwningPlayer = World ? World->GetFirstPlayerController() : nullptr;
        if (OwningPlayer && !OwningPlayer->IsLocalController())
        {
            return nullptr;
        }
    }

    return OwningPlayer ? OwningPlayer->GetPawn() : nullptr;
}
