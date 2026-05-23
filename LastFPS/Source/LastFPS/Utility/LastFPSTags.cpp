#include "Utility/LastFPSTags.h"
#include "GameplayTagsManager.h"

FLastFPSTags FLastFPSTags::Tags;

void FLastFPSTags::InitializeNativeTags()
{
    // 캐릭터 상태
    Tags.AddTag(Tags.Character_State_Dead, "Character.State.Dead", "Character is dead");
    Tags.AddTag(Tags.Character_State_Crouched, "Character.State.Crouched", "Character is crouching");

    // 쿨다운
    Tags.AddTag(Tags.Cooldown_Skill1, "Cooldown.Skill1", "Skill 1 cooldown");
    Tags.AddTag(Tags.Cooldown_Skill2, "Cooldown.Skill2", "Skill 2 cooldown");

    // UI
    Tags.AddTag(Tags.UI_HUD_Visible, "UI.HUD.Visible", "HUD visibility status");
}

void FLastFPSTags::AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment)
{
    OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TagName), FString(TagComment));
}
