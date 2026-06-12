#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "LastFPSTagFunctionLibrary.generated.h"

/**
 * 게임플레이 태그 관련 블루프린트 편의 기능을 제공하는 라이브러리
 */
UCLASS()
class LASTFPS_API ULastFPSTagFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Dash 쿨다운 태그 반환 */
    UFUNCTION(BlueprintPure, Category = "LastFPS|Tags")
    static FGameplayTag GetTag_CooldownSkillDash();

    /** 스킬 1 쿨다운 태그 반환 */
    UFUNCTION(BlueprintPure, Category = "LastFPS|Tags")
    static FGameplayTag GetTag_CooldownSkill1();

    /** 스킬 2 쿨다운 태그 반환 */
    UFUNCTION(BlueprintPure, Category = "LastFPS|Tags")
    static FGameplayTag GetTag_CooldownSkill2();

    /** 사망 상태 태그 반환 */
    UFUNCTION(BlueprintPure, Category = "LastFPS|Tags")
    static FGameplayTag GetTag_CharacterDead();

    /** 액터가 특정 게임플레이 태그를 가지고 있는지 확인 (AbilitySystemComponent 필요) */
    UFUNCTION(BlueprintCallable, Category = "LastFPS|Tags", meta = (DefaultToSelf = "TargetActor"))
    static bool ActorHasMatchingGameplayTag(AActor* TargetActor, FGameplayTag TagToCheck);
};
