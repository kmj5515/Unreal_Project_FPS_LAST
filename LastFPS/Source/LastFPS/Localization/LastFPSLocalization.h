#pragma once

#include "CoreMinimal.h"

class UEnum;

/** ST_Localize 문자열 테이블 에셋의 UI 텍스트 조회를 제공한다. */
class LASTFPS_API FLastFPSLocalization final
{
public:
	[[nodiscard]] static FText GetUIText(const TCHAR* Key);
	[[nodiscard]] static FText GetUIText(FName Key);
	[[nodiscard]] static FText GetUIEnumText(const UEnum* EnumType, int64 Value);
};

namespace LastFPSUIStringKeys
{
	inline constexpr TCHAR NoticeTitle[] = TEXT("Popup.Common.NoticeTitle");
	inline constexpr TCHAR QuitGameConfirmation[] = TEXT("Popup.QuitGame.Confirmation");
	inline constexpr TCHAR SettingsNoticeTitle[] = TEXT("Popup.Settings.Title");
	inline constexpr TCHAR SettingsNoticeBody[] = TEXT("Popup.Settings.UnavailableBody");
	inline constexpr TCHAR QuitGameTitle[] = TEXT("Popup.QuitGame.Title");
	inline constexpr TCHAR QuitGameMainMenuConfirmation[] = TEXT("Popup.QuitGame.MainMenuConfirmation");
	inline constexpr TCHAR ShopCannotBuyTitle[] = TEXT("Popup.Shop.CannotBuy.Title");
	inline constexpr TCHAR ShopCannotBuyBody[] = TEXT("Popup.Shop.CannotBuy.Body");
	inline constexpr TCHAR ShopBuyConfirmTitle[] = TEXT("Popup.Shop.Buy.Title");
	inline constexpr TCHAR QuestRewardTitle[] = TEXT("Popup.Quest.Reward.Title");
	inline constexpr TCHAR LobbyUnavailableFeature[] = TEXT("Popup.Lobby.UnavailableFeature");
	inline constexpr TCHAR BattleWeaponRequiredBody[] = TEXT("Popup.Battle.WeaponRequiredBody");

	inline constexpr TCHAR DialogueNext[] = TEXT("Dialogue.Action.Next");
	inline constexpr TCHAR DialogueClose[] = TEXT("Dialogue.Action.Close");
	inline constexpr TCHAR NPCDefaultName[] = TEXT("NPC.DefaultName");
	inline constexpr TCHAR NPCInteract[] = TEXT("NPC.Action.Interact");
	inline constexpr TCHAR NPCExit[] = TEXT("NPC.Action.Exit");
	inline constexpr TCHAR NPCNoActions[] = TEXT("NPC.Message.NoActions");

	inline constexpr TCHAR QuestStatusInProgress[] = TEXT("Quest.Status.InProgress");
	inline constexpr TCHAR QuestStatusCompleted[] = TEXT("Quest.Status.Completed");
	inline constexpr TCHAR QuestStatusClaimed[] = TEXT("Quest.Status.Claimed");
	inline constexpr TCHAR QuestStatusLocked[] = TEXT("Quest.Status.Locked");
	inline constexpr TCHAR QuestStatusNotStarted[] = TEXT("Quest.Status.NotStarted");
	inline constexpr TCHAR QuestRewardCreditsFormat[] = TEXT("Quest.Reward.CreditsFormat");
	inline constexpr TCHAR QuestRewardItemFormat[] = TEXT("Quest.Reward.ItemFormat");
	inline constexpr TCHAR QuestRewardMessageFormat[] = TEXT("Quest.Reward.MessageFormat");
	inline constexpr TCHAR WeaponTypeFormat[] = TEXT("Inventory.Weapon.TypeFormat");
	inline constexpr TCHAR QuantityMaxPurchaseFormat[] = TEXT("Quantity.MaxPurchaseFormat");

	inline constexpr TCHAR CountFormat[] = TEXT("Common.Format.Count");
	inline constexpr TCHAR RatioFormat[] = TEXT("Common.Format.Ratio");
	inline constexpr TCHAR IndexedLabelFormat[] = TEXT("Common.Format.IndexedLabel");
	inline constexpr TCHAR PercentFormat[] = TEXT("Common.Format.Percent");
	inline constexpr TCHAR DistanceMetersFormat[] = TEXT("Common.Format.DistanceMeters");
	inline constexpr TCHAR UnknownDistanceMeters[] = TEXT("Common.Distance.UnknownMeters");
	inline constexpr TCHAR PositiveValueFormat[] = TEXT("Common.Format.PositiveValue");
	inline constexpr TCHAR RangeFormat[] = TEXT("Common.Format.Range");
	inline constexpr TCHAR SecondsFormat[] = TEXT("Common.Format.Seconds");
	inline constexpr TCHAR MultiplierFormat[] = TEXT("Common.Format.Multiplier");
	inline constexpr TCHAR SectionHeaderFormat[] = TEXT("Common.Format.SectionHeader");
	inline constexpr TCHAR StatLineFormat[] = TEXT("Common.Format.StatLine");
	inline constexpr TCHAR ObjectiveProgressLineFormat[] = TEXT("Common.Format.ObjectiveProgressLine");
	inline constexpr TCHAR StatComparisonLineFormat[] = TEXT("Common.Format.StatComparisonLine");
	inline constexpr TCHAR TimerFormat[] = TEXT("Common.Format.Timer");
	inline constexpr TCHAR SpeakerNameFormat[] = TEXT("Common.Format.SpeakerName");
	inline constexpr TCHAR NotAvailable[] = TEXT("Common.Value.NotAvailable");

	inline constexpr TCHAR StatHealth[] = TEXT("Stat.Health");
	inline constexpr TCHAR StatStamina[] = TEXT("Stat.Stamina");
	inline constexpr TCHAR StatAttackDamage[] = TEXT("Stat.AttackDamage");
	inline constexpr TCHAR StatDefense[] = TEXT("Stat.Defense");
	inline constexpr TCHAR StatMoveSpeed[] = TEXT("Stat.MoveSpeed");

	inline constexpr TCHAR WeaponStatsAttackSection[] = TEXT("WeaponStats.Section.Attack");
	inline constexpr TCHAR WeaponStatsCriticalSection[] = TEXT("WeaponStats.Section.CriticalAndElement");
	inline constexpr TCHAR WeaponStatsAccuracySection[] = TEXT("WeaponStats.Section.AccuracyAndRange");
	inline constexpr TCHAR WeaponStatsMobilitySection[] = TEXT("WeaponStats.Section.Mobility");
	inline constexpr TCHAR WeaponStatsDamage[] = TEXT("WeaponStats.Label.Damage");
	inline constexpr TCHAR WeaponStatsFireRate[] = TEXT("WeaponStats.Label.FireRate");
	inline constexpr TCHAR WeaponStatsMagazineSize[] = TEXT("WeaponStats.Label.MagazineSize");
	inline constexpr TCHAR WeaponStatsReloadTime[] = TEXT("WeaponStats.Label.ReloadTime");
	inline constexpr TCHAR WeaponStatsPenetration[] = TEXT("WeaponStats.Label.Penetration");
	inline constexpr TCHAR WeaponStatsCriticalChance[] = TEXT("WeaponStats.Label.CriticalChance");
	inline constexpr TCHAR WeaponStatsCriticalMultiplier[] = TEXT("WeaponStats.Label.CriticalMultiplier");
	inline constexpr TCHAR WeaponStatsWeakpointMultiplier[] = TEXT("WeaponStats.Label.WeakpointMultiplier");
	inline constexpr TCHAR WeaponStatsMultiHitMultiplier[] = TEXT("WeaponStats.Label.MultiHitMultiplier");
	inline constexpr TCHAR WeaponStatsStatusEffectChance[] = TEXT("WeaponStats.Label.StatusEffectChance");
	inline constexpr TCHAR WeaponStatsBreakPower[] = TEXT("WeaponStats.Label.BreakPower");
	inline constexpr TCHAR WeaponStatsHipFireAccuracy[] = TEXT("WeaponStats.Label.HipFireAccuracy");
	inline constexpr TCHAR WeaponStatsAdsAccuracy[] = TEXT("WeaponStats.Label.AdsAccuracy");
	inline constexpr TCHAR WeaponStatsEffectiveRange[] = TEXT("WeaponStats.Label.EffectiveRange");
	inline constexpr TCHAR WeaponStatsMaxRange[] = TEXT("WeaponStats.Label.MaxRange");
	inline constexpr TCHAR WeaponStatsDamageFalloff[] = TEXT("WeaponStats.Label.DamageFalloff");
	inline constexpr TCHAR WeaponStatsMoveWhileFiring[] = TEXT("WeaponStats.Label.MoveWhileFiring");
	inline constexpr TCHAR WeaponStatsMoveWhileAiming[] = TEXT("WeaponStats.Label.MoveWhileAiming");
	inline constexpr TCHAR WeaponStatsSprintSpeed[] = TEXT("WeaponStats.Label.SprintSpeed");

	inline constexpr TCHAR TravelStatusMainMenu[] = TEXT("Travel.Status.MainMenu");
	inline constexpr TCHAR TravelStatusCharacterSelect[] = TEXT("Travel.Status.CharacterSelect");
	inline constexpr TCHAR TravelStatusHub[] = TEXT("Travel.Status.Hub");
	inline constexpr TCHAR TravelStatusGeneric[] = TEXT("Travel.Status.Generic");
	inline constexpr TCHAR TravelMapMainMenu[] = TEXT("Travel.Map.MainMenu");
	inline constexpr TCHAR TravelMapCharacterSelect[] = TEXT("Travel.Map.CharacterSelect");
	inline constexpr TCHAR TravelMapHub[] = TEXT("Travel.Map.Hub");
	inline constexpr TCHAR TravelUnsupportedEntryType[] = TEXT("Travel.Error.UnsupportedEntryType");
	inline constexpr TCHAR TravelInvalidMapId[] = TEXT("Travel.Error.InvalidMapId");
	inline constexpr TCHAR TravelInvalidMap[] = TEXT("Travel.Error.MapNotFound");
	inline constexpr TCHAR TravelInvalidWorld[] = TEXT("Travel.Error.InvalidWorld");
	inline constexpr TCHAR TravelWorldExpired[] = TEXT("Travel.Error.WorldExpired");
	inline constexpr TCHAR TravelFailure[] = TEXT("Travel.Error.Failure");

	inline constexpr TCHAR QuickPlayInvalidPlayer[] = TEXT("QuickPlay.Error.InvalidPlayer");
	inline constexpr TCHAR QuickPlayInvalidDefinitionId[] = TEXT("QuickPlay.Error.InvalidDefinitionId");
	inline constexpr TCHAR QuickPlayNoSessionSubsystem[] = TEXT("QuickPlay.Error.NoSessionSubsystem");
	inline constexpr TCHAR QuickPlayStateLost[] = TEXT("QuickPlay.Error.StateLost");
	inline constexpr TCHAR QuickPlayPlayerLost[] = TEXT("QuickPlay.Error.PlayerLost");

	inline constexpr TCHAR BattleLoadingIndicator[] = TEXT("Battle.Loading.Connecting");
	inline constexpr TCHAR BattleTravelStatus[] = TEXT("Battle.Loading.Travelling");
	inline constexpr TCHAR BattleDefinitionNotFound[] = TEXT("Battle.Error.DefinitionNotFound");
	inline constexpr TCHAR BattleDefinitionInvalid[] = TEXT("Battle.Error.DefinitionInvalid");
	inline constexpr TCHAR BattleMapInvalid[] = TEXT("Battle.Error.MapInvalid");
	inline constexpr TCHAR BattleSessionUnavailable[] = TEXT("Battle.Error.SessionUnavailable");
	inline constexpr TCHAR BattleSessionRequestInvalid[] = TEXT("Battle.Error.SessionRequestInvalid");
	inline constexpr TCHAR HostSessionFailed[] = TEXT("Battle.Error.HostSessionFailed");
	inline constexpr TCHAR JoinSessionFailed[] = TEXT("Battle.Error.JoinSessionFailed");
}
