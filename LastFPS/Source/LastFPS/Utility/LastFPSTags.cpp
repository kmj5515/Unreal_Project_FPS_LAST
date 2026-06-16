#include "Utility/LastFPSTags.h"
#include "GameplayTagsManager.h"

FLastFPSTags FLastFPSTags::Tags;

void FLastFPSTags::InitializeNativeTags()
{
    // 캐릭터 상태
    Tags.AddTag(Tags.Character_State_Dead,     "Character.State.Dead",     "Character is dead");
    Tags.AddTag(Tags.Character_State_Crouched, "Character.State.Crouched", "Character is crouching");

    // 쿨다운
    Tags.AddTag(Tags.Cooldown_Skill_Dash, "Cooldown.Skill.Dash", "Dash cooldown");
    Tags.AddTag(Tags.Cooldown_Skill1, "Cooldown.Skill1", "Skill 1 cooldown");
    Tags.AddTag(Tags.Cooldown_Skill2, "Cooldown.Skill2", "Skill 2 cooldown");
    Tags.AddTag(Tags.Cooldown_Ultimate, "Cooldown.Ultimate", "Ultimate cooldown");

    // UI
    Tags.AddTag(Tags.UI_HUD_Visible, "UI.HUD.Visible", "HUD visibility status");

    // 입력
    Tags.AddTag(Tags.Input_Move,        "InputTag.Move",        "Move input");
    Tags.AddTag(Tags.Input_Look,        "InputTag.Look",        "Look input");
    Tags.AddTag(Tags.Input_Sprint,      "InputTag.Sprint",      "Sprint input");
    Tags.AddTag(Tags.Input_ADS,         "InputTag.ADS",         "ADS input");
    Tags.AddTag(Tags.Input_Jump,        "InputTag.Jump",        "Jump input");
    Tags.AddTag(Tags.Input_Dash,        "InputTag.Dash",        "Dash input");
    Tags.AddTag(Tags.Input_Fire,        "InputTag.Fire",        "Fire input");
    Tags.AddTag(Tags.Input_Reload,      "InputTag.Reload",      "Reload input");
    Tags.AddTag(Tags.Input_Scoreboard,  "InputTag.Scoreboard",  "Scoreboard input");
    Tags.AddTag(Tags.Input_Skill1,      "InputTag.Skill1",      "Skill 1 input");
    Tags.AddTag(Tags.Input_Skill2,      "InputTag.Skill2",      "Skill 2 input");
    Tags.AddTag(Tags.Input_Ultimate,    "InputTag.Ultimate",    "Ultimate input");

    // 어빌리티
    Tags.AddTag(Tags.Ability_Sprint,   "Ability.Sprint",   "Sprint ability");
    Tags.AddTag(Tags.Ability_Jump,     "Ability.Jump",     "Jump ability");
    Tags.AddTag(Tags.Ability_Dash,     "Ability.Dash",     "Dash ability");
    Tags.AddTag(Tags.Ability_Fire,     "Ability.Fire",     "Fire ability");
    Tags.AddTag(Tags.Ability_Reload,   "Ability.Reload",   "Reload ability");
    Tags.AddTag(Tags.Ability_Skill1,   "Ability.Skill1",   "Skill 1 ability");
    Tags.AddTag(Tags.Ability_Skill2,   "Ability.Skill2",   "Skill 2 ability");
    Tags.AddTag(Tags.Ability_Ultimate, "Ability.Ultimate", "Ultimate ability");
}

void FLastFPSTags::AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment)
{
    OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TagName), FString(TagComment));
}
