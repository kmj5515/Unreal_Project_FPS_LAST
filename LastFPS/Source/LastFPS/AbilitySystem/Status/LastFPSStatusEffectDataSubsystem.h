#pragma once

#include "CoreMinimal.h"
#include "Data/Status/LastFPSStatusEffectUIData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LastFPSStatusEffectDataSubsystem.generated.h"

class UDataTable;

/** 상태 태그와 버프·디버프 HUD 표시 데이터를 연결하는 읽기 전용 레지스트리다. */
UCLASS(Config=Game)
class LASTFPS_API ULastFPSStatusEffectDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FLastFPSStatusEffectUIData* FindStatusEffectUIData(FGameplayTag StatusTag) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Status Effect")
	bool GetStatusEffectUIData(FGameplayTag StatusTag, FLastFPSStatusEffectUIData& OutData) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Status Effect")
	void GetVisibleStatusEffects(
		const FGameplayTagContainer& OwnedTags,
		TArray<FLastFPSStatusEffectUIData>& OutData) const;

	void GetRegisteredStatusTags(FGameplayTagContainer& OutTags) const;
	void GetStatusOverlayEntries(TArray<const FLastFPSStatusEffectUIData*>& OutEntries) const;
	void GetStatusAnimationEntries(TArray<const FLastFPSStatusEffectUIData*>& OutEntries) const;

	UFUNCTION(BlueprintPure, Category="LastFPS|Status Effect")
	TSoftObjectPtr<UTexture2D> GetFallbackIcon() const;

protected:
	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Status Effect")
	TSoftObjectPtr<UDataTable> StatusEffectUITable;

	UPROPERTY(Config, EditDefaultsOnly, Category="LastFPS|Status Effect")
	TSoftObjectPtr<UTexture2D> FallbackIcon;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedTable;

	TMap<FGameplayTag, const FLastFPSStatusEffectUIData*> DataByStatusTag;
};
