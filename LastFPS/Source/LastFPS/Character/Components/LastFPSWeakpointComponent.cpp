#include "Character/Components/LastFPSWeakpointComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ULastFPSWeakpointComponent::ULastFPSWeakpointComponent()
{
    // 피격 발광이 잦아드는 동안에만 틱한다. 평소엔 꺼둔다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    SetIsReplicatedByDefault(true);
}

void ULastFPSWeakpointComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULastFPSWeakpointComponent, WeakpointHealth);
}

void ULastFPSWeakpointComponent::BeginPlay()
{
    Super::BeginPlay();

    if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        TargetMesh = OwnerCharacter->GetMesh();
    }
    if (!TargetMesh.IsValid())
    {
        TargetMesh = GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
    }

    // 초기 체력은 만땅. 서버에서 변경되면 복제로 클라에 반영된다.
    WeakpointHealth = WeakpointMaxHealth;
    CurrentGlowIntensity = BaseGlowIntensity;

    EnsureGlowMID();
    ApplyGlow();
}

void ULastFPSWeakpointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    CurrentGlowIntensity = FMath::FInterpTo(CurrentGlowIntensity, BaseGlowIntensity, DeltaTime, FlashFadeSpeed);
    ApplyGlow();

    if (FMath::IsNearlyEqual(CurrentGlowIntensity, BaseGlowIntensity, 0.01f))
    {
        CurrentGlowIntensity = BaseGlowIntensity;
        ApplyGlow();
        SetComponentTickEnabled(false);
    }
}

bool ULastFPSWeakpointComponent::IsWeakpointBone(FName BoneName) const
{
    return !BoneName.IsNone() && WeakpointBones.Contains(BoneName);
}

float ULastFPSWeakpointComponent::HandleHitOnBone(FName HitBoneName, float Damage)
{
    // [진단] 임시 로그 — 호출 여부·본 이름·약점 여부·권한 확인. 문제 해결 후 제거.
    UE_LOG(LogTemp, Warning, TEXT("[Weakpoint] HandleHitOnBone Bone=%s 약점여부=%d Authority=%d"),
        *HitBoneName.ToString(), IsWeakpointBone(HitBoneName) ? 1 : 0, GetOwner() && GetOwner()->HasAuthority() ? 1 : 0);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return 1.f;
    }

    if (!IsWeakpointBone(HitBoneName))
    {
        return 1.f;
    }

    ApplyWeakpointDamage(Damage * DamageMultiplier);
    return DamageMultiplier;
}

void ULastFPSWeakpointComponent::ApplyWeakpointDamage(float Damage)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bBroken || Damage <= 0.f)
    {
        return;
    }

    WeakpointHealth = FMath::Max(WeakpointHealth - Damage, 0.f);
    if (WeakpointHealth <= 0.f)
    {
        bBroken = true;
    }

    // [진단] 임시 로그 — 약점 피해 적용 확인. 문제 해결 후 제거.
    UE_LOG(LogTemp, Warning, TEXT("[Weakpoint] 약점피해 %.1f -> 체력 %.1f/%.1f, MID=%s"),
        Damage, WeakpointHealth, WeakpointMaxHealth, GlowMID ? TEXT("있음") : TEXT("없음"));

    // 서버 로컬 표시 갱신 + 모든 클라에 피격 반짝임 전파. 색 단계는 체력 복제(OnRep)로 반영된다.
    ApplyGlow();
    Multicast_PlayHitFlash();
}

void ULastFPSWeakpointComponent::Multicast_PlayHitFlash_Implementation()
{
    StartFlash();
}

void ULastFPSWeakpointComponent::OnRep_WeakpointHealth()
{
    ApplyGlow();
}

void ULastFPSWeakpointComponent::StartFlash()
{
    CurrentGlowIntensity = HitFlashIntensity;
    ApplyGlow();
    SetComponentTickEnabled(true);
}

void ULastFPSWeakpointComponent::EnsureGlowMID()
{
    if (GlowMID)
    {
        return;
    }

    USkeletalMeshComponent* Mesh = TargetMesh.Get();
    if (!Mesh)
    {
        return;
    }

    // 슬롯의 현재 머티리얼(약점 파라미터를 가진 인스턴스)을 소스로 DMI 생성.
    GlowMID = Mesh->CreateDynamicMaterialInstance(WeakpointMaterialSlot);
}

void ULastFPSWeakpointComponent::ApplyGlow()
{
    EnsureGlowMID();
    if (!GlowMID)
    {
        return;
    }

    // 피격 순간(FlashAlpha=1)엔 단계 색을 흰색으로 블렌드해 "확 밝아지는" 느낌을 준다.
    const float WhitenAlpha = FMath::Clamp(GetFlashAlpha() * HitWhitenAmount, 0.f, 1.f);
    const FLinearColor DisplayColor = FMath::Lerp(ResolveStageColor(), HitFlashColor, WhitenAlpha);

    GlowMID->SetVectorParameterValue(GlowColorParameterName, DisplayColor);
    GlowMID->SetScalarParameterValue(GlowIntensityParameterName, CurrentGlowIntensity);
}

float ULastFPSWeakpointComponent::GetFlashAlpha() const
{
    const float Range = HitFlashIntensity - BaseGlowIntensity;
    if (Range <= KINDA_SMALL_NUMBER)
    {
        return 0.f;
    }
    return FMath::Clamp((CurrentGlowIntensity - BaseGlowIntensity) / Range, 0.f, 1.f);
}

float ULastFPSWeakpointComponent::GetStageAlpha() const
{
    if (WeakpointMaxHealth <= KINDA_SMALL_NUMBER)
    {
        return 1.f;
    }
    return FMath::Clamp(1.f - (WeakpointHealth / WeakpointMaxHealth), 0.f, 1.f);
}

FLinearColor ULastFPSWeakpointComponent::ResolveStageColor() const
{
    const float Alpha = GetStageAlpha();
    return Alpha < 0.5f
        ? FMath::Lerp(StageColorFull, StageColorMid, Alpha * 2.f)
        : FMath::Lerp(StageColorMid, StageColorBroken, Alpha * 2.f - 1.f);
}
