#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSDefendableDeviceActor.generated.h"

class UAbilitySystemComponent;
class ULastFPSAttributeSet;
class UStaticMeshComponent;

/** 장치 체력이 바뀔 때 (HUD 게이지 갱신용). Current/Max 는 절대값. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLastFPSDeviceHealthChanged, float, Current, float, Max);

/**
 * 수호(방어) 목표에서 지켜야 할 장치다.
 *
 * 체력을 자체 수치로 들지 않고 GAS 어트리뷰트(Health/MaxHealth)로 표현한다.
 * 덕분에 적의 공격 어빌리티와 GameplayEffect 경로를 그대로 재사용할 수 있고,
 * 추격·공격 BT 태스크도 대상이 Pawn 인지 묻지 않으므로 수정할 필요가 없다.
 *wwwwwwwww
 * 체력 초기값과 재시작 복구는 목표 정의 에셋의 초기화 효과가 소유한다 —
 * 밸런스 값을 레벨 배치 인스턴스에 박아두지 않기 위함이다.
 * 승패 판정은 이 액터가 아니라 ULastFPSTimedObjectiveComponent 가 소유한다.
 */
UCLASS()
class LASTFPS_API ALastFPSDefendableDeviceActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALastFPSDefendableDeviceActor();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 0~1 정규화 체력 (HUD 게이지·화면 마커용). */
	UFUNCTION(BlueprintPure, Category="LastFPS|Defend")
	float GetHealth01() const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Defend")
	bool IsDestroyed() const;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDeviceHealthChanged OnHealthChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 어트리뷰트 변경을 HUD 용 델리게이트로 옮긴다. 서버·클라 양쪽에서 동작한다. */
	void HandleHealthChanged(const struct FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const struct FOnAttributeChangeData& Data);
	void BroadcastHealth() const;

	UPROPERTY(VisibleAnywhere, Category="LastFPS|Defend")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/**
	 * 장치가 직접 소유하는 ASC.
	 * 캐릭터와 달리 PlayerState 가 없으므로 액터 자신이 소유하고 복제한다.
	 */
	UPROPERTY(VisibleAnywhere, Category="LastFPS|Defend")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULastFPSAttributeSet> AttributeSet;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
};
