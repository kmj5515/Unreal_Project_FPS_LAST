#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LastFPSSkillCooldownPresenter.generated.h"

class APlayerController;
class UAbilitySystemComponent;
class ULastFPSSkillCooldownSlotWidget;

/**
 * 스킬 쿨다운 슬롯(Q/E/Z/F) 표시를 HUD View에서 분리한다.
 * 캐릭터 Definition의 스킬 로드아웃으로 각 슬롯을 1회 구성하고, 매 갱신 주기에 ASC로부터 쿨다운을 반영한다.
 * Definition 로드아웃을 아직 못 읽으면 초기화를 미뤄 재시도할 수 있게 false를 반환한다.
 */
UCLASS()
class LASTFPS_API ULastFPSSkillCooldownPresenter final : public UObject
{
    GENERATED_BODY()

public:
    /** 바인딩된 4개 슬롯 위젯(null 허용)을 받아 구성한다. */
    void Initialize(
        ULastFPSSkillCooldownSlotWidget* InSlotQ,
        ULastFPSSkillCooldownSlotWidget* InSlotE,
        ULastFPSSkillCooldownSlotWidget* InSlotZ,
        ULastFPSSkillCooldownSlotWidget* InSlotF);

    /** 슬롯을 스킬 로드아웃으로 구성한다. 준비가 안 됐으면 false를 반환한다(재시도 대상). */
    bool TryInitialize(const APlayerController* OwningPlayer, UAbilitySystemComponent* ASC);

    bool IsInitialized() const { return bInitialized; }

    /** 각 슬롯의 쿨다운 표시를 ASC 기준으로 갱신한다. */
    void Tick(UAbilitySystemComponent* ASC);

private:
    UPROPERTY(Transient)
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> SlotQ;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> SlotE;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> SlotZ;

    UPROPERTY(Transient)
    TObjectPtr<ULastFPSSkillCooldownSlotWidget> SlotF;

    bool bInitialized = false;
};
