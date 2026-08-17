#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Character/LastFPSCharacterTypes.h"
#include "Data/Characters/LastFPSCharacterAcceleratorData.h"
#include "LastFPSCharacterDefinition.generated.h"

class APawn;
class UAbilitySystemComponent;
class ULastFPSAbilitySet;
class ULastFPSCharacterStatData;
class ULastFPSCharacterVisualData;
class UTexture2D;

/**
 * 플레이어와 적이 공유하는 캐릭터 정의의 베이스.
 * 플레이어 전용 데이터는 ULastFPSHeroDefinition,
 * 적 전용 데이터는 ULastFPSEnemyDefinition 에서 확장한다.
 *
 * NOTE: 기존 캐릭터 .uasset 들을 Hero/Enemy 서브클래스로 재지정(reparent)하기 전까지는
 *       이 클래스를 Abstract 로 두지 않는다. 마이그레이션이 끝나면 UCLASS(Abstract) 로 바꿀 것.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ── 공유 식별/분류 ─────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	ELastFPSCharacterType CharacterType = ELastFPSCharacterType::Player;

	/** 보스처럼 여러 시스템에서 공유하는 캐릭터 분류를 데이터로 표현한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(Categories="Character.Type"))
	FGameplayTagContainer ClassificationTags;

	UFUNCTION(BlueprintPure, Category="LastFPS|Character", meta=(Categories="Character.Type"))
	bool HasClassificationTag(FGameplayTag TagToCheck) const;

	/** 킬피드/체력바/보스 인트로 등 어디서나 쓰이므로 베이스에 둔다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character", meta=(AssetBundles="UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	// ── 공유 스폰/스탯/비주얼 ──────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Stats")
	TSoftObjectPtr<ULastFPSCharacterStatData> StatData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Visual")
	TSoftObjectPtr<ULastFPSCharacterVisualData> VisualData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Accelerator")
	TSoftObjectPtr<ULastFPSCharacterAcceleratorData> AcceleratorData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|GAS")
	TSoftObjectPtr<ULastFPSAbilitySet> AbilitySet;

	/**
	 * 다이어그램의 "Override function from base class".
	 * 베이스는 공통 스탯(StatData)을 ASC 에 적용한다.
	 * Hero/Enemy 는 Super 를 호출한 뒤 각자 전용 스타트업 처리를 덧붙인다.
	 *
	 * 어빌리티 부여는 서버 권한이 필요해 ALastFPSCharacterBase 에서 처리한다.
	 */
	virtual void GiveToAbilitySystem(UAbilitySystemComponent* ASC) const;

	/**
	 * 스폰 경로가 즉시 필요로 하는 소프트 참조 경로다.
	 * 이 경로들이 미리 상주해 있지 않으면 스폰 시점에 동기 로드로 떨어져 게임 스레드가 멈춘다.
	 * 파생 정의는 Super 를 호출한 뒤 자기 참조를 덧붙인다.
	 */
	virtual void GatherSpawnDependencyPaths(TArray<FSoftObjectPath>& OutPaths) const;
};
