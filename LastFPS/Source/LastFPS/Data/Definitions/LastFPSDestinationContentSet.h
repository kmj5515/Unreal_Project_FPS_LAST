#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/Definitions/LastFPSDestinationFeature.h"
#include "Engine/DataAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "LastFPSDestinationContentSet.generated.h"

/** 목적지 콘텐츠 준비 과정이 전체 로딩 바에서 차지할 비율과 내부 단계 가중치다. */
USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSDestinationLoadingProgressSettings
{
    GENERATED_BODY()

    /** 기존 레벨·컨트롤러·Pawn 준비를 제외한 목적지 콘텐츠의 전체 점유율이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading Progress",
        meta=(ClampMin="0.05", ClampMax="0.95"))
    float OverallProgressShare = 0.70f;

    /** Gameplay Cue와 해당 Niagara 의존성은 콘텐츠 에셋 단계에 포함한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading Progress",
        meta=(ClampMin="0.0"))
    float AssetAndGameplayCueWeight = 0.40f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading Progress",
        meta=(ClampMin="0.0"))
    float ActorPoolWeight = 0.20f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading Progress",
        meta=(ClampMin="0.0"))
    float RenderComponentWeight = 0.15f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading Progress",
        meta=(ClampMin="0.0"))
    float ShaderAndPSOWeight = 0.25f;
};

USTRUCT(BlueprintType)
struct LASTFPS_API FLastFPSRenderWarmupSettings
{
    GENERATED_BODY()

    /** 실제 플레이어 Pawn의 렌더 컴포넌트와 PSO가 준비될 때까지 로딩 화면을 유지한다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Render Warmup")
    bool bEnabled = true;

    /**
     * 렌더 스레드가 등록된 컴포넌트를 처리할 시간을 보장한다.
     * 셰이더·PSO 작업이 끝난 뒤에도 이 프레임 수만큼 연속으로 안정 상태여야 완료된다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Render Warmup",
        meta=(ClampMin="1", UIMin="1", UIMax="10"))
    int32 MinimumStableFrames = 3;

    /** 예기치 않은 컴파일 실패로 로딩 화면이 영구 유지되지 않게 하는 상한이다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Render Warmup",
        meta=(ClampMin="1.0", UIMin="1.0", UIMax="30.0", Units="s"))
    float TimeoutSeconds = 10.0f;
};

/**
 * 맵 진입 전에 준비되어야 하는 콘텐츠 목록.
 * "이 맵이 무엇을 요구하는가"는 맵마다 다른 규칙이므로 GameMode 가 이 에셋을 가리킨다.
 * (InitialScreenTag / LevelRestrictionEffect 와 같은 소유 규칙)
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSDestinationContentSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    static const FPrimaryAssetType PrimaryAssetType;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    /** 팝업과 같은 로컬 콘텐츠 수명 정책이 현재 목적지를 식별할 때 사용한다. */
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category="Context",
        meta=(Categories="Game.Context"))
    FGameplayTagContainer ContextTags;

    /** 데이터 테이블·텍스처·데이터 에셋 등. 로드가 끝날 때까지 로딩 화면을 유지한다. */
    UPROPERTY(EditDefaultsOnly, Category="Content")
    TArray<TSoftObjectPtr<UObject>> RequiredAssets;

    /**
     * 위젯·Pawn 같은 Blueprint 클래스는 반드시 이쪽에 넣는다.
     * RequiredAssets 에 넣으면 생성 클래스(_C)가 아니라 UBlueprint 를 가리켜
     * 쿠킹된 빌드에서 해석되지 않는다.
     */
    UPROPERTY(EditDefaultsOnly, Category="Content")
    TArray<TSoftClassPtr<UObject>> RequiredClasses;

    /**
     * 목적지 기능별 데이터 계약이다.
     * 각 기능은 중앙 로딩에 필요한 경로를 공통 계약으로 제공한다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Features")
    TArray<TObjectPtr<ULastFPSDestinationFeature>> Features;

    /** 에셋 로드 다음 단계에서 수행할 렌더 준비 정책. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading")
    FLastFPSRenderWarmupSettings RenderWarmup;

    /** 로딩 화면의 실제 진행률에 적용할 단계별 가중치다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Loading")
    FLastFPSDestinationLoadingProgressSettings LoadingProgress;

    /** 직접 목록과 기능 계약의 유효한 경로를 모은다. 비어 있으면 게이트가 걸리지 않는다. */
    void CollectRequiredPaths(TArray<FSoftObjectPath>& OutPaths) const;

    template <typename TFeature>
    TFeature* FindFeature()
    {
        for (const TObjectPtr<ULastFPSDestinationFeature>& Feature : Features)
        {
            if (TFeature* TypedFeature = Cast<TFeature>(Feature.Get()))
            {
                return TypedFeature;
            }
        }
        return nullptr;
    }

    template <typename TFeature>
    const TFeature* FindFeature() const
    {
        for (const TObjectPtr<ULastFPSDestinationFeature>& Feature : Features)
        {
            if (const TFeature* TypedFeature = Cast<TFeature>(Feature.Get()))
            {
                return TypedFeature;
            }
        }
        return nullptr;
    }
};
