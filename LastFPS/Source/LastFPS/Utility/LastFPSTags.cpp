#include "Utility/LastFPSTags.h"

namespace LastFPSGameplayTags
{
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dead, "Character.State.Dead", "Character is dead")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Crouched, "Character.State.Crouched", "Character is crouching")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_AuraDamageCooldown, "Status.AuraDamageCooldown", "오라 데미지 재적용 대기 상태")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Freeze, "Status.Freeze", "빙결 상태")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_FreezeStack, "Status.FreezeStack", "빙결 누적 상태")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Dash, "Cooldown.Skill.Dash", "Dash cooldown")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill1, "Cooldown.Skill1", "Skill 1 cooldown")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill2, "Cooldown.Skill2", "Skill 2 cooldown")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill3, "Cooldown.Skill3", "Skill 3 cooldown")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Ultimate, "Cooldown.Ultimate", "Ultimate cooldown")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "SetByCaller damage magnitude")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CriticalHit, "SetByCaller.CriticalHit", "SetByCaller critical hit flag")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_AbilityCommit, "Event.Montage.Ability.Commit", "Commit an ability from an ability montage")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_AbilityEnd, "Event.Montage.Ability.End", "End an ability from an ability montage")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_ProjectileSpawn, "Event.Montage.Projectile.Spawn", "Spawn a projectile from an ability montage")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_ViolaIceAuraEffect, "Event.Montage.ViolaIceAura.Effect", "비올라 얼음 오라 이펙트 실행")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_ViolaFrostStormEffect, "Event.Montage.ViolaFrostStorm.Effect", "비올라 냉기 폭풍 효과 실행")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_IceStormSpawn, "Event.Montage.IceStorm.Spawn", "Spawn an ice storm area from an ability montage")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_HUD_Visible, "UI.HUD.Visible", "HUD visibility status")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Move, "InputTag.Move", "Move input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Look, "InputTag.Look", "Look input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Sprint, "InputTag.Sprint", "Sprint input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_ADS, "InputTag.ADS", "ADS input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Jump, "InputTag.Jump", "Jump input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Dash, "InputTag.Dash", "Dash input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Fire, "InputTag.Fire", "Fire input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Reload, "InputTag.Reload", "Reload input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Scoreboard, "InputTag.Scoreboard", "Scoreboard input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill1, "InputTag.Skill1", "Skill 1 input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill2, "InputTag.Skill2", "Skill 2 input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill3, "InputTag.Skill3", "Skill 3 input")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ultimate, "InputTag.Ultimate", "Ultimate input")

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Sprint, "Ability.Sprint", "Sprint ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Jump, "Ability.Jump", "Jump ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_DoubleJump, "Ability.DoubleJump", "Double jump ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dash, "Ability.Dash", "Dash ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire, "Ability.Fire", "Fire ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reload, "Ability.Reload", "Reload ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill1, "Ability.Skill1", "Skill 1 ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill2, "Ability.Skill2", "Skill 2 ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill3, "Ability.Skill3", "Skill 3 ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Ultimate, "Ability.Ultimate", "Ultimate ability")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ProbePassive, "Ability.Passive.Probe", "프로브 패시브 어빌리티")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Trigger_SpawnProbe, "Ability.Trigger.SpawnProbe", "프로브 생성 트리거 어빌리티")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Shoot, "Ability.Enemy.Shoot", "적 AI 원거리 발사 공격")
}
