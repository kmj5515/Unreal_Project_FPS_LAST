#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LastFPSDefendableDeviceActor.generated.h"

class UStaticMeshComponent;

/** 방어 대상 장치의 내구도가 바뀔 때 (HUD 게이지 갱신용). Current/Max 는 절대값. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLastFPSDeviceIntegrityChanged, float, Current, float, Max);

/** 내구도가 0 이 되어 장치가 파괴된 순간 (방어 실패 판정 소스). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLastFPSDeviceDestroyed);

/**
 * 퍼스트디센던트식 수호(방어) 목표에서 "지켜야 할 장치/오브젝트".
 * 캐릭터가 아닌 무생물 대상이라 GAS 체력 대신 자체 내구도(Integrity)만 서버 권한으로 소유한다.
 * 데미지 소스(적 공격)는 프로젝트마다 다르므로 ApplyIntegrityDamage 를 열어 두고, 실제 연결은
 * 적 AI/BP 브릿지가 담당한다. 승패 판정은 이 액터가 아니라 ULastFPSDefendObjectiveComponent 가 소유.
 */
UCLASS()
class LASTFPS_API ALastFPSDefendableDeviceActor : public AActor
{
	GENERATED_BODY()

public:
	ALastFPSDefendableDeviceActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 내구도 감소 (서버 권한). 0 도달 시 OnDeviceDestroyed 를 1회 브로드캐스트. */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Defend")
	void ApplyIntegrityDamage(float Amount);

	/** 내구도를 최대치로 복구 (방어 재시도 리셋용, 서버 권한). */
	UFUNCTION(BlueprintCallable, Category="LastFPS|Defend")
	void ResetIntegrity();

	/** 0~1 정규화 내구도 (HUD 게이지용). */
	UFUNCTION(BlueprintPure, Category="LastFPS|Defend")
	float GetIntegrity01() const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Defend")
	bool IsDestroyed() const { return Integrity <= 0.f; }

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDeviceIntegrityChanged OnIntegrityChanged;

	UPROPERTY(BlueprintAssignable, Category="LastFPS|Defend")
	FOnLastFPSDeviceDestroyed OnDeviceDestroyed;

protected:
	virtual void BeginPlay() override;

	/** 장치 최대 내구도 (밸런스 값 — 인스턴스별 조정). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LastFPS|Defend", meta=(ClampMin="1.0"))
	float MaxIntegrity = 1000.f;

private:
	UFUNCTION()
	void OnRep_Integrity();

	/** 내구도 변경 후 공통 통지 — 게이지 갱신 + (0 도달 시 1회) 파괴 통지. 서버·클라 양쪽에서 호출. */
	void HandleIntegrityUpdated();

	UPROPERTY(VisibleAnywhere, Category="LastFPS|Defend")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing=OnRep_Integrity)
	float Integrity = 0.f;

	/** 파괴 통지 1회 래치 (서버·클라 각자 소유 — 복제 안 함). */
	bool bDestroyedBroadcast = false;
};
