#pragma once

#include "CoreMinimal.h"
#include "EasyCrosshairSystem/ecsCrosshairBehavior.h"
#include "LastFPSCrosshairSpreadBehavior.generated.h"

class APawn;

/**
 * 이동·낙하·ADS 상태에 따라 크로스헤어 레이어를 벌리는 ecs Behavior.
 *
 * SetLayerTransform은 레이어의 기본 변환을 덮어쓰는 절대값 방식이므로,
 * BeginBehavior에서 에셋의 기본 변환을 캐시한 뒤 "기본 위치 + 방향 × 스프레드"로 적용한다.
 *
 * bDriveFromWeaponSpread가 켜져 있으면 기본 벌어짐·ADS 수렴·연사 누적을 자체 px 수치 대신
 * WeaponComponent의 실제 탄퍼짐 반각을 화면 px로 투영해 사용한다. 크로스헤어와 탄착이 어긋나지
 * 않게 하려면 이쪽이 기본이고, 아래 px 수치들은 무기가 없을 때의 폴백으로 남는다.
 *
 * 연사 누적 벌어짐(발사마다 가산·상한 클램프·시간 감쇠)은 고정 커브인 ecs 애니메이션으로
 * 표현할 수 없고, 위젯 틱 순서상 Behavior가 애니메이션의 레이어 변환을 덮어쓰므로
 * NotifyWeaponFired 누적값으로 여기서 처리한다. ecs 애니메이션(FireAnimationName)은
 * 스프레드 레이어와 겹치지 않는 연출(색 플래시 등)에만 사용한다.
 *
 * 무기별로 반응이 달라야 하면 코드 수정 없이 크로스헤어 에셋에 인스턴스된
 * 이 Behavior의 수치만 다르게 설정한다.
 *
 * [수명 전제] ecs는 에셋에 인스턴스된 Behavior를 위젯에 그대로 연결하지만, 이 프로젝트는
 * LastFPSEasyCrosshairPresenter가 에셋의 런타임 임시 사본을 만들어 사용하므로 이 객체는
 * 위젯과 수명을 같이하는 사본 인스턴스로 동작한다(공유 에셋 오염 없음).
 * 방어적으로 모든 런타임 상태는 BeginBehavior에서 초기화한다.
 */
// 확장은 C++ 파생(CalculateTargetSpreadPx override)으로만 한다 — BP 파생 차단
UCLASS(NotBlueprintable)
class LASTFPS_API ULastFPSCrosshairSpreadBehavior : public UecsCrosshairBehavior
{
    GENERATED_BODY()

public:
    ULastFPSCrosshairSpreadBehavior();

    virtual void BeginBehavior() override;
    virtual void Tick() override;

    /** 발사 1회 통지 — 누적 벌어짐 가산(상한 클램프). Presenter가 발사 시 호출한다. */
    void NotifyWeaponFired();

protected:
    /** 스프레드를 적용할 레이어와 벌어질 방향(단위 벡터 권장). 에셋의 레이어 키와 일치해야 한다. */
    UPROPERTY(EditAnywhere, Category="Spread|Layers")
    TMap<FName, FVector2D> SpreadLayerDirections;

    /** 정지 상태에서의 기본 벌어짐(px) */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.0"))
    float BaseSpreadPx = 0.f;

    /** 수평 이동 중 추가 벌어짐(px) */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.0"))
    float MoveSpreadPx = 12.f;

    /** 낙하(점프) 중 추가 벌어짐(px) */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.0"))
    float JumpSpreadPx = 24.f;

    /** 이동으로 판정할 수평 속도 임계값(cm/s) */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.0"))
    float MovementSpeedThreshold = 10.f;

    /** ADS 중 스프레드 배율 (0=완전 수렴). 무기 퍼짐 연동 시에는 무기 데이터의 ADS 배율이 대신 적용된다. */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.0", ClampMax="1.0"))
    float AdsSpreadMultiplier = 0.3f;

    /**
     * 무기의 실제 탄퍼짐 반각을 화면 px로 투영해 기본 벌어짐으로 사용한다.
     * 끄면 BaseSpreadPx/AdsSpreadMultiplier/FireSpread* 의 표시 전용 수치만 쓴다(기존 동작).
     */
    UPROPERTY(EditAnywhere, Category="Spread|Weapon")
    bool bDriveFromWeaponSpread = true;

    /** 투영값 보정 배율 — 조준선 두께나 아트 여백 때문에 딱 맞지 않을 때 눈으로 맞추는 손잡이 */
    UPROPERTY(EditAnywhere, Category="Spread|Weapon", meta=(ClampMin="0.0"))
    float WeaponSpreadPxScale = 1.f;

    /** 목표 스프레드로의 보간 속도 */
    UPROPERTY(EditAnywhere, Category="Spread", meta=(ClampMin="0.01"))
    float InterpSpeed = 12.f;

    /** 발사 1회당 추가 벌어짐(px) */
    UPROPERTY(EditAnywhere, Category="Spread|Fire", meta=(ClampMin="0.0"))
    float FireSpreadAddPx = 8.f;

    /** 연사 누적 벌어짐 상한(px) */
    UPROPERTY(EditAnywhere, Category="Spread|Fire", meta=(ClampMin="0.0"))
    float FireSpreadMaxPx = 28.f;

    /** 발사 누적분이 0으로 돌아오는 감쇠 속도 */
    UPROPERTY(EditAnywhere, Category="Spread|Fire", meta=(ClampMin="0.01"))
    float FireSpreadRecoverSpeed = 9.f;

    /**
     * 목표 스프레드 계산. 파생 C++ 클래스가 조건을 확장할 수 있는 유일한 지점.
     * FireBloomPx는 표시 전용 연사 누적분이며, 무기 퍼짐 연동 시에는 무기가 이미 누적을 갖고 있어 무시된다.
     */
    virtual float CalculateTargetSpreadPx(const APawn& Pawn, float FireBloomPx) const;

private:
    void ApplySpreadToLayers(float SpreadPx);
    APawn* ResolveOwningPawnSafe() const;
    /** 무기 퍼짐 반각(도)을 화면 px로 투영. 무기가 없거나 연동이 꺼져 있으면 음수를 돌려준다. */
    float CalculateWeaponSpreadPx(const APawn& Pawn) const;

    /** 에셋에 정의된 레이어 기본 변환 캐시 — 절대값 교체 방식 보정용 */
    struct FCachedLayerBase
    {
        FVector2D Direction = FVector2D::ZeroVector;
        FVector2D BaseTransform = FVector2D::ZeroVector;
        float BaseRotation = 0.f;
        float BaseScale = 0.f;
    };
    TMap<FName, FCachedLayerBase> CachedLayerBases;

    /** TickRate 설정과 무관하게 정확한 경과 시간을 얻기 위한 자체 타임스탬프 */
    double LastTickSeconds = 0.0;

    float CurrentSpreadPx = 0.f;
    float LastAppliedSpreadPx = -1.f;

    /** 연사 누적 벌어짐 — 발사마다 가산되고 매 틱 감쇠한다 */
    float FireSpreadPx = 0.f;
};
