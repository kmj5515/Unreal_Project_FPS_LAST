#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "LastFPSCharacterRoster.generated.h"

class ULastFPSCharacterDefinition;
class APawn;

/**
 * 선택 가능한 캐릭터 목록의 단일 소스(Single Source of Truth).
 * GameInstance가 config 경로로 이 에셋 하나를 가리키고,
 * 위젯·PlayerController·GameMode가 모두 여기서 읽는다.
 * 배열 인덱스 = 캐릭터 선택 인덱스(Card_0=0, Card_1=1, ...).
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterRoster : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Roster", meta=(AssetBundles="Game"))
	TArray<TSoftObjectPtr<ULastFPSCharacterDefinition>> Characters;

	int32 Num() const { return Characters.Num(); }

	/** 범위 밖이면 nullptr */
	const ULastFPSCharacterDefinition* GetDefinition(int32 Index) const;

	/** 해당 정의의 PawnClass. 정의가 없으면 nullptr */
	TSoftClassPtr<APawn> GetPawnClass(int32 Index) const;
};
