import unreal


MAP_PATH = "/Game/Maps/Network/MasterLobbyMap"
EXPECTED_GAME_MODE_PATH = "/Script/LastFPS.LastFPSMasterLobbyGameMode"


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not asset_subsystem.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"레벨 에셋이 없습니다: {MAP_PATH}")

if not level_subsystem.load_level(MAP_PATH):
    raise RuntimeError(f"레벨을 열지 못했습니다: {MAP_PATH}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not world:
    raise RuntimeError("에디터 월드를 찾지 못했습니다.")

game_mode_class = world.get_world_settings().get_editor_property("default_game_mode")
if not game_mode_class or game_mode_class.get_path_name() != EXPECTED_GAME_MODE_PATH:
    actual_path = game_mode_class.get_path_name() if game_mode_class else "None"
    raise RuntimeError(f"GameMode가 일치하지 않습니다: {actual_path}")

actors = actor_subsystem.get_all_level_actors()
floor_count = sum(1 for actor in actors if actor.get_actor_label() == "MasterLobby_Floor")
player_starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]

if floor_count != 1:
    raise RuntimeError(f"로비 바닥 개수가 올바르지 않습니다: {floor_count}")
if len(player_starts) != 4:
    raise RuntimeError(f"PlayerStart 개수가 올바르지 않습니다: {len(player_starts)}")

unreal.log(
    f"마스터 로비 레벨 검증 완료: Map={MAP_PATH}, "
    f"GameMode={game_mode_class.get_path_name()}, Floor={floor_count}, PlayerStarts={len(player_starts)}"
)
