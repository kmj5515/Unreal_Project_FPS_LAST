#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSGrapplingReticlePresenter.generated.h"

class UImage;
class UOverlay;
class UWidgetTree;

/** 그래플링 조준점의 크기·색 설정. 디자이너 노출 값은 View에서 관리하고 초기화 시 전달한다. */
struct FLastFPSGrapplingReticleConfig
{
    float DotSize = 6.f;
    float IdleScale = 1.f;
    float AvailableScale = 1.7f;
    float ScaleInterpSpeed = 12.f;
    FLinearColor IdleColor = FLinearColor(1.f, 1.f, 1.f, 0.55f);
    FLinearColor AvailableColor = FLinearColor(0.15f, 0.9f, 1.f, 1.f);
};

/**
 * EasyCrosshair 아래에서 항상 표시되는 그래플링 가능 상태 점을 HUD View에서 분리한다.
 * 바인딩 위젯이 없으면 크로스헤어 호스트 안에 런타임으로 생성한다.
 */
UCLASS()
class LASTFPS_API ULastFPSGrapplingReticlePresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 바인딩 조준점 위젯(null 허용), 위젯 트리, 설정을 받아 구성한다. */
    void Initialize(UImage* InDotImage, UWidgetTree* InWidgetTree, const FLastFPSGrapplingReticleConfig& InConfig);

    /** 조준점이 없으면 호스트 안에 생성하고 초기 시각 상태를 1회 구성한다. */
    void EnsureDot(UOverlay* Host);

    /** 그래플링 대상 유무에 따라 목표 크기·색을 갱신한다. */
    void SetAvailability(bool bTargetAvailable);

    /** 조준점 크기를 목표값으로 매끄럽게 보간한다. */
    void Tick(float DeltaTime);

    /** 바인딩·상태를 초기화한다. View의 NativeDestruct에서 호출한다. */
    void Reset();

private:
    UPROPERTY(Transient)
    TObjectPtr<UImage> DotImage;

    TWeakObjectPtr<UWidgetTree> WidgetTree;
    TWeakObjectPtr<UOverlay> HostOverlay;

    FLastFPSGrapplingReticleConfig Config;

    float CurrentScale = 1.f;
    float TargetScale = 1.f;
    bool bInitialized = false;
};
