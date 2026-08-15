import unreal


MAP_PATH = "/Game/Maps/Network/MasterLobbyMap"
# 로비는 네이티브 마스터 로비 GameMode를 직접 사용한다. 기존 BP_GameModeBase는
# 게임플레이 데이터 참조를 통째로 끌고 들어와 서버 cook과 기동 로그를 불필요하게 키운다.
GAME_MODE_CLASS_PATH = "/Script/LastFPS.LastFPSMasterLobbyGameMode"


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

if asset_subsystem.does_asset_exist(MAP_PATH):
    raise RuntimeError(f"이미 레벨이 존재합니다: {MAP_PATH}")

require(level_subsystem.new_level(MAP_PATH), f"레벨 생성에 실패했습니다: {MAP_PATH}")

world = require(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), "에디터 월드를 찾지 못했습니다.")
world_settings = require(world.get_world_settings(), "WorldSettings를 찾지 못했습니다.")
game_mode_class = require(unreal.load_class(None, GAME_MODE_CLASS_PATH), f"GameMode 클래스를 찾지 못했습니다: {GAME_MODE_CLASS_PATH}")
world_settings.set_editor_property("default_game_mode", game_mode_class)

floor_mesh = require(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"), "기본 Cube 메시를 찾지 못했습니다.")
floor = require(
    actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, -50.0)),
    "로비 바닥 생성에 실패했습니다.",
)
floor.set_actor_label("MasterLobby_Floor")
floor.set_actor_scale3d(unreal.Vector(20.0, 20.0, 1.0))
floor.static_mesh_component.set_static_mesh(floor_mesh)
floor.static_mesh_component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)

start_locations = (
    unreal.Vector(-300.0, -300.0, 100.0),
    unreal.Vector(300.0, -300.0, 100.0),
    unreal.Vector(-300.0, 300.0, 100.0),
    unreal.Vector(300.0, 300.0, 100.0),
)

for index, location in enumerate(start_locations, start=1):
    player_start = require(
        actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, location),
        f"PlayerStart {index} 생성에 실패했습니다.",
    )
    player_start.set_actor_label(f"MasterLobby_PlayerStart_{index:02d}")
    player_start.set_editor_property("player_start_tag", unreal.Name(f"Lobby.Start.{index:02d}"))

require(level_subsystem.save_current_level(), f"레벨 저장에 실패했습니다: {MAP_PATH}")
unreal.log(f"마스터 로비 레벨 생성 완료: {MAP_PATH}")
