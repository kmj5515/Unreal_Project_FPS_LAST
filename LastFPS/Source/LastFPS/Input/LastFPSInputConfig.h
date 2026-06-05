#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LastFPSInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FLastFPSInputAction
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<const UInputAction> InputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(Categories="InputTag"))
    FGameplayTag InputTag;

};

// DataAsset으로 만들어서 BP에서 InputAction ↔ GameplayTag 매핑을 에디터에서 편집
UCLASS(BlueprintType, Const)
class LASTFPS_API ULastFPSInputConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    const UInputAction* FindNativeInputActionByTag(const FGameplayTag& Tag) const;
    const UInputAction* FindAbilityInputActionByTag(const FGameplayTag& Tag) const;

    // 이동·시점·달리기 등 네이티브 핸들러로 직접 처리하는 액션
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(TitleProperty="InputAction"))
    TArray<FLastFPSInputAction> NativeInputActions;

    // GAS 어빌리티를 트리거하는 액션 (Q / E / F / 사격 / 점프)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(TitleProperty="InputAction"))
    TArray<FLastFPSInputAction> AbilityInputActions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(Categories="InputTag"))
    FGameplayTagContainer ReleaseCancelInputTags;
};
