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

    // 캐릭터 상태 태그
    FGameplayTag Character_State_Dead;
    FGameplayTag Character_State_Crouched;

    // 쿨다운 태그
    FGameplayTag Cooldown_Skill1;
    FGameplayTag Cooldown_Skill2;

    // 기타 시스템 태그
    FGameplayTag UI_HUD_Visible;

protected:
    void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment);

private:
    static FLastFPSTags Tags;
};
