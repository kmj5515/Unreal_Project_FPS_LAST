#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSStatusOverlayConfig.generated.h"

class UGameplayEffect;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FLastFPSStatusOverlayMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	FGameplayTag StatusTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	FGameplayTag StackTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	TSubclassOf<UGameplayEffect> StackEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	FName StackMixParameterName = TEXT("VfxMix");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	float StackMixStartValue = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	float StackMixEndValue = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	bool bInterpolateStackMix = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay", meta=(ClampMin="0.0"))
	float StackMixInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	int32 Priority = 0;
};

/** 기존 Overlay Data Asset을 DT_StatusData로 옮기는 동안만 에셋을 열기 위해 유지한다. 마이그레이션 후 제거한다. */
UCLASS(BlueprintType, meta=(DeprecatedNode, DeprecationMessage="DT_StatusData의 Overlay 필드를 사용하세요."))
class LASTFPS_API ULastFPSStatusOverlayConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LastFPS|Status Overlay")
	TArray<FLastFPSStatusOverlayMaterial> OverlayMaterials;
};
