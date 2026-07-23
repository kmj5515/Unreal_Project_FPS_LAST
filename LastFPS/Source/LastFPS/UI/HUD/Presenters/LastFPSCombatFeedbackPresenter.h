#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "LastFPSCombatFeedbackPresenter.generated.h"

class AActor;
class APlayerController;
class UImage;
class UMaterialInstanceDynamic;
class UOverlay;
class ULastFPSDamageDirectionIndicatorWidget;
class ULastFPSDamageNumberWidget;

/**
 * 히트마커·데미지 넘버·피격 방향 표시 설정.
 * 디자이너 노출 값은 View(HUD 위젯)의 EditDefaultsOnly 멤버에서 관리하고,
 * 초기화 시 이 구조체로 Presenter에 전달한다. Presenter는 로직만 소유한다.
 */
struct FLastFPSCombatFeedbackConfig
{
    FName HitMarkerSpreadParameterName = TEXT("HitSpread");
    float HitMarkerMaxSpread = 5.f;
    float HitMarkerSpreadExpandDuration = 0.12f;

    TSubclassOf<ULastFPSDamageNumberWidget> DamageNumberWidgetClass;
    FVector DamageNumberWorldOffset = FVector(0.f, 0.f, 55.f);
    FVector2D DamageNumberScreenOffset = FVector2D(90.f, -20.f);
    float DamageNumberRandomRadius = 24.f;
    float DamageNumberRandomRadiusOffset = 0.f;

    TSubclassOf<ULastFPSDamageDirectionIndicatorWidget> DamageDirectionIndicatorWidgetClass;
    int32 MaxDamageDirectionIndicators = 6;
};

/**
 * 전투 피드백(적을 맞혔을 때/맞았을 때의 화면 표시)을 HUD View에서 분리한다.
 * 히트마커 스프레드 애니메이션, 데미지 숫자 스폰, 피격 방향 인디케이터를 소유한다.
 * View는 바인딩 위젯과 설정을 넘겨 초기화한 뒤 이벤트·틱만 위임한다.
 */
UCLASS()
class LASTFPS_API ULastFPSCombatFeedbackPresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 바인딩 위젯과 설정을 받아 초기 상태를 구성한다. 바인딩 위젯은 선택적이라 null을 허용한다. */
    void Initialize(UImage* InHitMarkerImage, UOverlay* InDamageDirectionLayer, const FLastFPSCombatFeedbackConfig& InConfig);

    /** 등록한 인디케이터를 정리한다. View의 NativeDestruct에서 호출한다. */
    void Shutdown();

    /** 히트마커를 표시하고 스프레드 애니메이션을 시작한다. */
    void ShowHitMarker();

    /** 월드 기준 공격 방향을 화면 방향 인디케이터로 표시한다. */
    void ShowDamageDirection(APlayerController* OwningPlayer, const FVector& DamageSourceDirection);

    /** 데미지 숫자 위젯을 스폰한다. DamageAmount가 0 이하이면 무시한다. */
    void SpawnDamageNumber(
        APlayerController* OwningPlayer,
        float DamageAmount,
        float TotalDamageDealt,
        const FVector& DamageWorldLocation,
        AActor* DamageTargetActor,
        bool bCriticalHit);

    /** 히트마커 스프레드와 방향 인디케이터를 매 HUD 갱신 주기마다 진행한다. */
    void Tick(APlayerController* OwningPlayer, float DeltaTime);

private:
    void InitializeHitMarkerMaterial();
    void SetHitMarkerSpread(float Spread);
    void TickHitMarkerSpread(float DeltaTime);
    void HideHitMarker();

    void TickDamageDirectionIndicators(APlayerController* OwningPlayer, float DeltaTime);
    void ClearDamageDirectionIndicators();

    FVector2D MakeDamageNumberRandomOffset() const;

    UPROPERTY(Transient)
    TObjectPtr<UImage> HitMarkerImage;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> DamageDirectionIndicatorLayer;

    UPROPERTY(Transient)
    TArray<TObjectPtr<ULastFPSDamageDirectionIndicatorWidget>> ActiveDamageDirectionIndicators;

    TWeakObjectPtr<UMaterialInstanceDynamic> HitMarkerMaterial;

    FLastFPSCombatFeedbackConfig Config;

    float HitMarkerSpreadElapsed = 0.f;
    bool bHitMarkerSpreadAnimating = false;

    /** 설정 누락 경고를 한 번만 남겨 정상 프레임에서 반복 로그가 발생하지 않도록 한다. */
    bool bDamageDirectionConfigurationWarningLogged = false;
};
