#pragma once

#include "CoreMinimal.h"
#include "Data/Definitions/LastFPSCharacterDefinition.h"
#include "LastFPSHeroDefinition.generated.h"

/**
 * 플레이어(히어로) 전용 캐릭터 정의.
 * 캐릭터 선택 화면에서만 쓰이는 표시용 데이터(Role/Description)를 담는다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSHeroDefinition : public ULastFPSCharacterDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero")
	FText Role;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero", meta=(MultiLine=true))
	FText Description;

	virtual void GiveToAbilitySystem(UAbilitySystemComponent* ASC) const override;
};
