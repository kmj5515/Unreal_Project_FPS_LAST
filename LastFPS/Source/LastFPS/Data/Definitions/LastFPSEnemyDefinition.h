#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "LastFPSEnemyDefinition.generated.h"

class ULastFPSAIProfile;
class ULastFPSWeaponDefinition;

/**
 * 적(Enemy) 전용 캐릭터 정의.
 * AI 행동 프로파일 등 적에게만 필요한 데이터를 담는다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSEnemyDefinition : public ULastFPSCharacterDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|AI")
	TSoftObjectPtr<ULastFPSAIProfile> AIProfile;

	/** 적이 스폰될 때 서버에서 자동 장착할 초기 무기다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Weapon")
	TSoftObjectPtr<ULastFPSWeaponDefinition> InitialWeaponDefinition;

	virtual void GiveToAbilitySystem(UAbilitySystemComponent* ASC) const override;

	virtual void GatherSpawnDependencyPaths(TArray<FSoftObjectPath>& OutPaths) const override;

	/**
	 * 초기 무기 정의가 이미 로드된 경우에만, 그 무기가 장착 시점에 동기 해석하는 참조를 모은다.
	 * 무기 정의 자체가 소프트 참조라 GatherSpawnDependencyPaths 단계에서는 아직 알 수 없다.
	 * 프리로드 2단계에서 호출한다.
	 */
	void GatherInitialWeaponDependencyPaths(TArray<FSoftObjectPath>& OutPaths) const;
};
