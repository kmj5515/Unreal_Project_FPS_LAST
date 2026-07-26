#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LastFPSWeakpointComponent.generated.h"

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

/**
 * 적의 약점(예: 머리) 표시를 담당한다.
 * 지정한 머티리얼 슬롯을 런타임 DMI로 만들어, 약점 체력에 따라 발광 색을
 * 파랑(만땅)→노랑→빨강(파괴 직전)으로 바꾸고, 피격 순간 강도를 튀겼다 복귀시킨다.
 *
 * 히트 판정·데미지 적용은 이 컴포넌트가 소유하지 않는다. 무기/데미지 경로에서 피격 본 이름을
 * 넘겨 HandleHitOnBone을 호출하면, 약점 본과 일치할 때만 약점 체력·발광이 갱신되고
 * 데미지 배수를 반환한다(호출측이 실제 피해에 곱해 적용).
 * 시각 표현은 서버 권한으로 갱신하고 복제한다.
 */
UCLASS(ClassGroup=(LastFPS), meta=(BlueprintSpawnableComponent))
class LASTFPS_API ULastFPSWeakpointComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULastFPSWeakpointComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /**
     * 피격 본이 약점 본과 일치하면 약점 체력을 깎고 발광을 갱신한다(서버 권한).
     * @return 약점 히트였으면 데미지 배수, 아니면 1.0. 호출측이 실제 피해에 곱해 쓴다.
     */
    UFUNCTION(BlueprintCallable, Category="Weakpoint")
    float HandleHitOnBone(FName HitBoneName, float Damage);

    /** 본 무관하게 약점에 직접 피해를 준다(디버그·직접 호출용, 서버 권한). */
    UFUNCTION(BlueprintCallable, Category="Weakpoint")
    void ApplyWeakpointDamage(float Damage);

    UFUNCTION(BlueprintPure, Category="Weakpoint")
    bool IsWeakpointBone(FName BoneName) const;

    UFUNCTION(BlueprintPure, Category="Weakpoint")
    bool IsBroken() const { return bBroken; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** 발광시킬 머티리얼 슬롯(엘리먼트) 인덱스. ToiletMech는 약점(Toilet) 슬롯이 1이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint")
    int32 WeakpointMaterialSlot = 1;

    /** 약점으로 판정할 본 목록. 피격 본이 이 중 하나면 약점 히트로 처리한다.
     *  (이 메시는 머리 지오메트리가 head가 아닌 spine_05/neck에 스킨돼 있어 목록으로 둔다.) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint")
    TArray<FName> WeakpointBones = { TEXT("head"), TEXT("neck_02"), TEXT("neck_01"), TEXT("spine_05") };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint", meta=(ClampMin="1.0"))
    float WeakpointMaxHealth = 100.f;

    /** 약점 히트가 실제 피해에 곱하는 배수. HandleHitOnBone이 반환한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint", meta=(ClampMin="1.0"))
    float DamageMultiplier = 2.f;

    // ── 머티리얼 파라미터 이름(M_SciFi_ToiletMech_Toilet_Skin2에 추가한 것과 일치해야 한다) ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Material")
    FName GlowColorParameterName = TEXT("WeakpointGlowColor");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Material")
    FName GlowIntensityParameterName = TEXT("WeakpointGlow");

    // ── 단계별 색(HDR). 체력 100%→Full, 50%→Mid, 0%→Broken 으로 보간 ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Color")
    FLinearColor StageColorFull = FLinearColor(0.f, 0.6f, 6.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Color")
    FLinearColor StageColorMid = FLinearColor(6.f, 4.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Color")
    FLinearColor StageColorBroken = FLinearColor(6.f, 0.f, 0.f);

    // ── 발광 강도 ──
    /** 평소 은은한 발광 강도. 0이면 맞을 때만 반짝인다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Glow", meta=(ClampMin="0.0"))
    float BaseGlowIntensity = 1.f;

    /** 피격 순간 튀는 최대 발광 강도. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Glow", meta=(ClampMin="0.0"))
    float HitFlashIntensity = 8.f;

    /** 피격 발광이 BaseGlowIntensity로 돌아오는 속도(클수록 빨리 잦아듦). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Glow", meta=(ClampMin="0.1"))
    float FlashFadeSpeed = 6.f;

    /** 피격 순간 색이 잠깐 이 색으로 블렌드된다(기본 흰색). 강도 flash와 함께 잦아든다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Glow")
    FLinearColor HitFlashColor = FLinearColor::White;

    /** 피격 순간 흰색으로 블렌드되는 정도(0=색 변화 없음, 1=완전 흰색). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weakpoint|Glow", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HitWhitenAmount = 1.f;

private:
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayHitFlash();

    UPROPERTY(ReplicatedUsing=OnRep_WeakpointHealth)
    float WeakpointHealth = 0.f;

    UFUNCTION()
    void OnRep_WeakpointHealth();

    void EnsureGlowMID();
    /** 현재 단계 색(피격 시 흰색 블렌드 포함)과 강도를 MID에 함께 반영한다. */
    void ApplyGlow();
    void StartFlash();

    /** 0(체력 만땅)~1(파괴 직전). */
    float GetStageAlpha() const;
    FLinearColor ResolveStageColor() const;

    /** 0(잦아듦)~1(피격 순간). 강도 flash를 정규화한 값으로 흰색 블렌드에 쓴다. */
    float GetFlashAlpha() const;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GlowMID;

    TWeakObjectPtr<USkeletalMeshComponent> TargetMesh;

    float CurrentGlowIntensity = 0.f;
    bool bBroken = false;
};
