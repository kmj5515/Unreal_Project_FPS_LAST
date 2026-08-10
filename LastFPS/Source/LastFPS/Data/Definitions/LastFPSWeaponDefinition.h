#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Utility/LastFPSEnumTypes.h"
#include "LastFPSWeaponDefinition.generated.h"

class ALastFPSProjectile;
class ALastFPSWeaponActor;
class AActor;
class UAnimationAsset;
class UAnimInstance;
class UCameraShakeBase;
class UecsCrosshairEditorAsset;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;
class UTexture2D;
class UUserWidget;

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponCrosshairSettings
{
    GENERATED_BODY()

    /** 무기별 EasyCrosshair 에셋이다. 비어 있으면 HUD의 기본 에셋을 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair")
    TSoftObjectPtr<UecsCrosshairEditorAsset> CrosshairAsset;

    /** 발사 시 실행할 EasyCrosshair 애니메이션 이름이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair")
    FName FireAnimationName = TEXT("Shoot");

    /** 0이면 EasyCrosshair 에셋에 저장된 애니메이션 시간을 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.0", Units="s"))
    float FireAnimationDuration = 0.f;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponAimRecoilSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", Units="deg"))
    float Strength = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0"))
    float FirstShotStrengthMultiplier = 1.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", Units="s"))
    float FirstShotResetInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0"))
    float HorizontalRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RandomnessRatio = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0"))
    float ADSMultiplier = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.01"))
    float InterpolationSpeed = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RecoveryRatio = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim Recoil", meta=(ClampMin="0.01"))
    float RecoveryInterpolationSpeed = 10.f;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponMagazineVisualSettings
{
    GENERATED_BODY()

    /** 손에 표시할 탄창 BP입니다. 즉시 사용하는 시각 요소이므로 Definition과 함께 로드합니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine")
    TSubclassOf<AActor> DetachedMagazineActorClass;

    /** 분리 시 숨길 무기 Skeletal Mesh의 탄창 본입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine")
    FName WeaponMagazineBoneName = TEXT("Clip_Bone");

    /** 분리된 탄창을 부착할 캐릭터 메시의 손 소켓입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine")
    FName HandSocketName = TEXT("MagazineHandSocket");

    /** 손 소켓에 스냅한 뒤 적용할 무기별 상대 변환입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine")
    FTransform HandAttachmentOffset = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSWeaponScopeSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope")
    bool bUseScope = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope", meta=(EditCondition="bUseScope", ClampMin="5.0", ClampMax="170.0", Units="deg"))
    float ScopeFOV = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope", meta=(EditCondition="bUseScope", ClampMin="0.1", ClampMax="3.0"))
    float ScopeSensitivityMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope", meta=(EditCondition="bUseScope", ClampMin="0.0"))
    float ScopeInterpSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scope", meta=(EditCondition="bUseScope"))
    TSoftClassPtr<UUserWidget> ScopeOverlayWidgetClass;
};

UCLASS(BlueprintType)
class LASTFPS_API ULastFPSWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    static const FPrimaryAssetType PrimaryAssetType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    FName WeaponId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    FText DisplayName;

    /**
     * HUD 무기 슬롯에 표시할 아이콘.
     * DT_ItemData 의 아이콘과 별개로 무기 정의가 소유해, HUD 가 복제된 무기 정의만으로 표시를 완성할 수 있게 한다
     * (아이템 행을 역으로 찾지 않아도 되고, 픽업으로 주운 무기처럼 아이템 행이 없는 경우도 표시된다).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(AssetBundles="UI"))
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    EMMWeaponType WeaponType = EMMWeaponType::Unarmed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Crosshair")
    FLastFPSWeaponCrosshairSettings Crosshair;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TSoftClassPtr<ALastFPSWeaponActor> WeaponActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
    TSoftClassPtr<ALastFPSProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TSoftClassPtr<UAnimInstance> AnimLayerClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation")
    TSoftObjectPtr<UAnimationAsset> FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Animation", meta=(ClampMin="0.01"))
    float AnimationPlayRate = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName MuzzleSocketName = TEXT("Projectile_Start");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName AttachSocketName = TEXT("WeaponSocket");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName LeftHandIKSocketName = TEXT("LeftHandIK");

    /** 비전투 준비 자세에서 기본 왼손 IK 소켓 대신 사용할 무기 소켓이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName ReadyLeftHandIKSocketName = TEXT("LeftHandIK_Ready");

    /** 일반 무기 왼손 Two Bone IK의 팔꿈치 목표를 계산할 기준 본이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|IK")
    FName LeftHandIKJointRootBoneName = TEXT("upperarm_l");

    /** 기준 본 위치에 더할 메시 컴포넌트 공간 팔꿈치 목표 오프셋이다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|IK", meta=(Units="cm"))
    FVector LeftHandIKJointTargetOffset = FVector(-15.f, -40.f, 5.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Sockets")
    FName ReloadLeftHandIKTargetName = TEXT("Clip_Bone");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Reload")
    FLastFPSWeaponMagazineVisualSettings MagazineVisual;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TSoftObjectPtr<USoundBase> FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects")
    TSoftObjectPtr<UParticleSystem> MuzzleFlashEffect;

	/** 로컬 소유자가 발사할 때 재생할 무기 전용 카메라 셰이크입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects|Camera Shake")
	TSoftClassPtr<UCameraShakeBase> FireCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Effects|Camera Shake", meta=(ClampMin="0.0"))
	float FireCameraShakeScale = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Recoil")
    FLastFPSWeaponAimRecoilSettings AimRecoil;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Scope")
    FLastFPSWeaponScopeSettings Scope;

};
