#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "LastFPSEnemyDefinition.generated.h"

class ULastFPSAIProfile;

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
	TObjectPtr<ULastFPSAIProfile> AIProfile;

	virtual void GiveToAbilitySystem(UAbilitySystemComponent* ASC) const override;
};
