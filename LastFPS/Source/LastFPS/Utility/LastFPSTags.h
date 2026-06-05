#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * 프로젝트 내의 모든 Native Gameplay Tags를 중앙 집중식으로 관리하는 싱글톤 구조체
 */
struct FLastFPSTags
{
public:
    static const FLastFPSTags& Get() { return Tags; }
    static void InitializeNativeTags();

    // 캐릭터 상태
    FGameplayTag Character_State_Dead;
    FGameplayTag Character_State_Crouched;

    // 쿨다운
    FGameplayTag Cooldown_Skill1;
    FGameplayTag Cooldown_Skill2;

    // UI
    FGameplayTag UI_HUD_Visible;

    // 입력
    FGameplayTag Input_Move;
    FGameplayTag Input_Look;
    FGameplayTag Input_Sprint;
    FGameplayTag Input_ADS;
    FGameplayTag Input_Jump;
    FGameplayTag Input_Fire;
    FGameplayTag Input_Reload;
    FGameplayTag Input_Scoreboard;
    FGameplayTag Input_Skill1;
    FGameplayTag Input_Skill2;
    FGameplayTag Input_Ultimate;

    // 어빌리티
    FGameplayTag Ability_Sprint;
    FGameplayTag Ability_Jump;
    FGameplayTag Ability_Fire;
    FGameplayTag Ability_Reload;
    FGameplayTag Ability_Skill1;
    FGameplayTag Ability_Skill2;
    FGameplayTag Ability_Ultimate;

protected:
    void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment);

private:
    static FLastFPSTags Tags;
};
