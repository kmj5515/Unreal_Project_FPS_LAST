"""퀘스트 컷신을 마스터 + 샷 서브시퀀스 구조로 생성한다.

에디터 Python 콘솔 또는 MCP execute_python 에서 실행한다. 대상 맵이 열려 있어야
카메라 기준 액터를 찾을 수 있다.

구성 원칙
- 마스터는 샷의 순서와 길이만 소유한다. 샷 내용을 고치지 않고 편집만으로 리듬을 바꾼다.
- 샷 하나는 렌즈 하나, 움직임 하나로 단순하게 둔다. 줌(초점거리 변화)은 쓰지 않고 컷으로 넘긴다.
- 구도는 화면 비율(-0.5~0.5)로 지정하고 렌즈 화각으로 각도를 역산한다. 렌즈를 바꿔도 구도가 유지된다.
- 카메라 좌표는 기준 액터의 트랜스폼과 메시 높이에서 계산한다. 레벨이 바뀌면 값이 따라온다.
"""

import math

import unreal


DISPLAY_RATE = unreal.FrameRate(30, 1)

# 카메라 궤적 샘플 간격(프레임). 촘촘할수록 주시·핸드헬드가 정확하고 키가 늘어난다.
KEY_INTERVAL_FRAMES = 5

# 컷 진입/이탈 페이드 길이(초).
FADE_SECONDS = 0.25


CINEMATIC_SPECS = [
    {
        "master_name": "LS_Q_Mobility_Start",
        "folder": "/Game/Cinematics/Quest/Q_Mobility",
        "shot_prefix": "LS_QMob",
        "focus_actor_class": "BP_NPC_ModuleManager",
        "shots": [
            {
                # 상황 제시 — 창구 전경. 거의 고정으로 두고 아주 느리게만 민다.
                "name": "0010_Establish",
                "duration_seconds": 1.8,
                "focal_length": 24.0,
                "aperture": 8.0,
                "start_coverage": 0.46,   # 대상이 화면 높이의 46% — 주변 상황이 함께 보인다
                "end_coverage": 0.49,
                "start_yaw_offset": 34.0,
                "end_yaw_offset": 31.0,
                "start_height_ratio": 1.55,
                "end_height_ratio": 1.50,
                "aim_height_ratio": 0.62,
                "compose_x": -0.17,   # 대상을 좌측 1/3 로 밀어 우측에 공간을 남긴다
                "compose_y": 0.02,
                "ease": "linear",
                "handheld": 0.04,
            },
            {
                # 접근 — 짧은 호를 그리며 다가가고 끝에서 감속해 정착한다.
                "name": "0020_Medium",
                "duration_seconds": 1.7,
                "focal_length": 35.0,
                "aperture": 4.0,
                "start_coverage": 0.95,   # 허리 위
                "end_coverage": 1.12,
                "start_yaw_offset": 24.0,
                "end_yaw_offset": 14.0,
                "start_height_ratio": 1.12,
                "end_height_ratio": 1.06,
                "aim_height_ratio": 0.80,
                "compose_x": 0.14,    # 시선 방향(좌측)에 여백을 주려고 우측 1/3 배치
                "compose_y": -0.04,
                "ease": "out",
                "handheld": 0.09,
            },
            {
                # 대사 받는 앵글 — 사실상 고정. 핸드헬드만 살짝 남긴다.
                "name": "0030_Close",
                "duration_seconds": 1.5,
                "focal_length": 50.0,
                "aperture": 2.8,
                "start_coverage": 1.95,   # 머리와 어깨
                "end_coverage": 2.10,
                "start_yaw_offset": -6.0,
                "end_yaw_offset": -9.0,
                "start_height_ratio": 1.00,
                "end_height_ratio": 0.99,
                "aim_height_ratio": 0.93,
                "compose_x": -0.10,
                "compose_y": -0.07,   # 머리 위 여백을 남긴다
                "ease": "in_out",
                "handheld": 0.16,
            },
        ],
    },
    {
        "master_name": "LS_Quest_Hunt_Start",
        "folder": "/Game/Cinematics/Quest/Quest_Hunt",
        "shot_prefix": "LS_QHunt",
        "focus_actor_class": "BP_NPC_Officer",
        "shots": [
            {
                # 전선 상황 제시 — 장교와 지휘 구역을 함께 보여 주며 천천히 접근한다.
                "name": "0010_CommandPost",
                "duration_seconds": 1.9,
                "focal_length": 24.0,
                "aperture": 5.6,
                "start_coverage": 0.42,
                "end_coverage": 0.47,
                "start_yaw_offset": 43.0,
                "end_yaw_offset": 36.0,
                "start_height_ratio": 1.58,
                "end_height_ratio": 1.48,
                "aim_height_ratio": 0.62,
                "compose_x": -0.18,
                "compose_y": 0.02,
                "ease": "out",
                "handheld": 0.03,
            },
            {
                # 작전 브리핑 — 허리 위 구도로 짧은 호를 그리며 명령 전달에 집중한다.
                "name": "0020_Briefing",
                "duration_seconds": 1.8,
                "focal_length": 35.0,
                "aperture": 4.0,
                "start_coverage": 0.88,
                "end_coverage": 1.08,
                "start_yaw_offset": 25.0,
                "end_yaw_offset": 15.0,
                "start_height_ratio": 1.14,
                "end_height_ratio": 1.07,
                "aim_height_ratio": 0.80,
                "compose_x": 0.14,
                "compose_y": -0.03,
                "ease": "out",
                "handheld": 0.06,
            },
            {
                # 출동 명령 — 머리와 어깨를 잡고 거의 고정해 마지막 지시의 무게를 살린다.
                "name": "0030_Order",
                "duration_seconds": 1.8,
                "focal_length": 50.0,
                "aperture": 2.8,
                "start_coverage": 1.78,
                "end_coverage": 1.96,
                "start_yaw_offset": -9.0,
                "end_yaw_offset": -6.0,
                "start_height_ratio": 1.02,
                "end_height_ratio": 1.00,
                "aim_height_ratio": 0.93,
                "compose_x": -0.10,
                "compose_y": -0.06,
                "ease": "in_out",
                "handheld": 0.08,
            },
        ],
    },
]


# ── 계산 헬퍼 ────────────────────────────────────────────────────────────────

def lerp(start, end, alpha):
    return start + (end - start) * alpha


def apply_ease(alpha, mode):
    """샷의 속도 곡선. 등속은 기계처럼 보이므로 기본은 감속 도착이다."""
    if mode == "out":
        return 1.0 - (1.0 - alpha) ** 3
    if mode == "in_out":
        return alpha * alpha * (3.0 - 2.0 * alpha)
    return alpha


def half_fov_tangent(sensor_size_mm, focal_length_mm):
    return sensor_size_mm / (2.0 * focal_length_mm)


def distance_for_coverage(coverage, actor_height, sensor_height_mm, focal_length_mm):
    """대상이 화면 높이의 coverage 만큼을 채우도록 하는 카메라 거리(cm).

    거리를 손으로 적어두면 렌즈를 바꿀 때마다 구도가 깨진다. 구도(=화면을 얼마나 채우는가)를
    의도로 두고 거리는 렌즈에서 역산한다.
    """
    visible_height = actor_height / coverage
    return (visible_height / 2.0) / half_fov_tangent(sensor_height_mm, focal_length_mm)


def compose_offset_degrees(screen_fraction, sensor_size_mm, focal_length_mm):
    """화면 비율(0=중앙, ±0.5=화면 끝) 구도를 렌즈 화각 기준 각도로 바꾼다."""
    return math.degrees(
        math.atan(2.0 * screen_fraction * half_fov_tangent(sensor_size_mm, focal_length_mm)))


def handheld_offset(seconds, amplitude):
    """저주파 두 개를 겹쳐 사람이 든 카메라의 미세한 흔들림을 만든다.

    난수 대신 결정적 파형을 쓴다 — 다시 생성해도 같은 결과가 나와야 리뷰가 가능하다.
    """
    yaw = amplitude * (math.sin(seconds * 2.1) * 0.6 + math.sin(seconds * 3.7 + 1.3) * 0.4)
    pitch = amplitude * (math.sin(seconds * 1.7 + 0.6) * 0.6 + math.sin(seconds * 4.3) * 0.4)
    return yaw, pitch


def distance_between(a, b):
    return math.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2)


def orbit_location(center, distance, yaw_degrees, world_z):
    radians = math.radians(yaw_degrees)
    return unreal.Vector(
        center.x + distance * math.cos(radians),
        center.y + distance * math.sin(radians),
        world_z,
    )


def look_at_rotation(from_location, to_location):
    delta_x = to_location.x - from_location.x
    delta_y = to_location.y - from_location.y
    delta_z = to_location.z - from_location.z
    horizontal = math.sqrt(delta_x * delta_x + delta_y * delta_y)
    return math.degrees(math.atan2(delta_y, delta_x)), math.degrees(math.atan2(delta_z, horizontal))


# ── 레벨 조회 ────────────────────────────────────────────────────────────────

def require_focus_actor(class_fragment):
    for actor in unreal.EditorActorSubsystem().get_all_level_actors():
        if class_fragment in actor.get_class().get_name():
            return actor
    raise RuntimeError(
        f"기준 액터를 찾지 못했습니다: '{class_fragment}'. 대상 맵이 열려 있는지 확인하십시오.")


def actor_vertical_extent(actor):
    """액터가 실제로 그려지는 높이 구간(바닥 Z, 정수리 Z).

    액터 원점은 캡슐 중심이라 캐릭터 키와 무관하다. 구도를 키에 맞추려면 메시 바운드를 쓴다.
    """
    for component in actor.get_components_by_class(unreal.SkeletalMeshComponent):
        mesh = component.get_editor_property("skeletal_mesh_asset")
        if not mesh:
            continue
        bounds = mesh.get_bounds()
        base_z = component.get_world_location().z + bounds.origin.z
        return base_z - bounds.box_extent.z, base_z + bounds.box_extent.z
    raise RuntimeError(f"기준 액터에서 스켈레탈 메시를 찾지 못했습니다: {actor.get_actor_label()}")


# ── 시퀀스 조립 ──────────────────────────────────────────────────────────────

def create_sequence(folder, asset_name, overwrite_existing):
    asset_path = f"{folder}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not overwrite_existing:
            raise RuntimeError(
                f"기존 시퀀스를 보호하기 위해 생성을 중단했습니다: {asset_path}. "
                "덮어쓰려면 run(..., overwrite_existing=True)를 명시하십시오.")
        # 명시적으로 허용한 경우에만 이전 키가 겹치지 않도록 지우고 새로 만든다.
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, folder, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    if not sequence:
        raise RuntimeError(f"레벨 시퀀스를 만들지 못했습니다: {asset_path}")
    sequence.set_display_rate(DISPLAY_RATE)
    return sequence


def add_camera_bindings(sequence):
    """스포너블 카메라와 카메라 컴포넌트 바인딩을 만든다.

    컴포넌트 바인딩은 시퀀스 내부 템플릿으로는 만들어지지 않아(5.7 확인), 레벨에 임시
    카메라를 세워 바인딩을 뜬 뒤 그 액터를 지운다.
    """
    actor_subsystem = unreal.EditorActorSubsystem()
    temp_camera = actor_subsystem.spawn_actor_from_class(
        unreal.CineCameraActor, unreal.Vector(0.0, 0.0, 0.0))
    try:
        camera_binding = sequence.add_spawnable_from_instance(temp_camera)
        component_binding = sequence.add_possessable(temp_camera.camera_component)
        if not camera_binding.is_valid() or not component_binding.is_valid():
            raise RuntimeError("카메라 바인딩을 만들지 못했습니다.")

        # add_possessable 이 만든 소유 액터 바인딩은 부모를 옮긴 뒤 지운다(빈 트랙 방지).
        auto_parent = component_binding.get_parent()
        component_binding.set_parent(camera_binding)
        if auto_parent.is_valid() and auto_parent != camera_binding:
            auto_parent.remove()

        return camera_binding, component_binding
    finally:
        actor_subsystem.destroy_actor(temp_camera)


def configure_camera_template(camera_binding, shot):
    """샷의 렌즈와 노출을 카메라 기본값으로 굽는다.

    초점거리를 트랙으로 흔들면 줌이 되어 버리므로 샷 안에서는 고정값으로 둔다.
    """
    component = camera_binding.get_object_template().camera_component
    component.set_editor_property("current_focal_length", shot["focal_length"])
    component.set_editor_property("current_aperture", shot["aperture"])

    # 노출 잠금(EV100)은 선택 사항이다. 컷마다 밝기가 출렁이는 것을 막지만, 레벨의
    # 포스트프로세스 볼륨이 노출을 소유하고 있으면 카메라 설정이 묻히므로 기본은 끈다.
    exposure_ev = shot.get("exposure_lock_ev")
    if exposure_ev is not None:
        post_process = component.get_editor_property("post_process_settings")
        post_process.set_editor_property("b_override_eye_adaptation_min_brightness", True)
        post_process.set_editor_property("b_override_eye_adaptation_max_brightness", True)
        post_process.set_editor_property("auto_exposure_min_brightness", float(exposure_ev))
        post_process.set_editor_property("auto_exposure_max_brightness", float(exposure_ev))
        component.set_editor_property("post_process_settings", post_process)

    return component.get_editor_property("filmback")


def add_transform_keys(section, frame, location, yaw, pitch):
    channels = section.get_channels_by_type(unreal.MovieSceneScriptingDoubleChannel)
    values = [location.x, location.y, location.z, 0.0, pitch, yaw]
    for channel, value in zip(channels, values):
        channel.add_key(
            unreal.FrameNumber(frame), value,
            interpolation=unreal.MovieSceneKeyInterpolation.AUTO)


def add_float_track(binding, property_path, end_frame, keys):
    track = binding.add_track(unreal.MovieSceneFloatTrack)
    if not track:
        raise RuntimeError(f"속성 트랙을 만들지 못했습니다: {property_path}")
    track.set_property_name_and_path(property_path.rsplit(".", 1)[-1], property_path)
    section = track.add_section()
    section.set_range(0, end_frame)
    channel = section.get_channels_by_type(unreal.MovieSceneScriptingFloatChannel)[0]
    for frame, value in keys:
        channel.add_key(
            unreal.FrameNumber(frame), value,
            interpolation=unreal.MovieSceneKeyInterpolation.AUTO)
    return track


def build_shot(folder, shot, focus_actor, shot_prefix, overwrite_existing):
    center = focus_actor.get_actor_location()
    base_yaw = focus_actor.get_actor_rotation().yaw
    ground_z, head_z = actor_vertical_extent(focus_actor)
    actor_height = head_z - ground_z

    sequence = create_sequence(
        f"{folder}/Shots", f"{shot_prefix}_{shot['name']}", overwrite_existing)
    sequence.set_playback_start_seconds(0.0)
    sequence.set_playback_end_seconds(shot["duration_seconds"])
    end_frame = int(round(shot["duration_seconds"] * DISPLAY_RATE.numerator / DISPLAY_RATE.denominator))

    camera_binding, component_binding = add_camera_bindings(sequence)
    filmback = configure_camera_template(camera_binding, shot)

    # 구도 오프셋 — 화면 비율을 이 샷의 렌즈 화각으로 각도 환산한다.
    yaw_compose = compose_offset_degrees(
        shot["compose_x"], filmback.sensor_width, shot["focal_length"])
    pitch_compose = compose_offset_degrees(
        shot["compose_y"], filmback.sensor_height, shot["focal_length"])

    transform_track = camera_binding.add_track(unreal.MovieScene3DTransformTrack)
    transform_section = transform_track.add_section()
    transform_section.set_range(0, end_frame)

    # 주시 지점 — 샷마다 다르다. 와이드는 몸통, 클로즈는 머리를 잡아야 구도가 성립한다.
    focus_point = unreal.Vector(
        center.x, center.y, ground_z + actor_height * shot["aim_height_ratio"])

    previous_yaw = None
    focus_distance_keys = []
    for frame in range(0, end_frame + 1, KEY_INTERVAL_FRAMES):
        raw_alpha = frame / end_frame
        alpha = apply_ease(raw_alpha, shot["ease"])
        seconds = frame / (DISPLAY_RATE.numerator / DISPLAY_RATE.denominator)

        coverage = lerp(shot["start_coverage"], shot["end_coverage"], alpha)
        distance = distance_for_coverage(
            coverage, actor_height, filmback.sensor_height, shot["focal_length"])
        yaw_offset = lerp(shot["start_yaw_offset"], shot["end_yaw_offset"], alpha)
        height_ratio = lerp(shot["start_height_ratio"], shot["end_height_ratio"], alpha)
        location = orbit_location(
            center, distance, base_yaw + yaw_offset, ground_z + actor_height * height_ratio)

        yaw, pitch = look_at_rotation(location, focus_point)
        handheld_yaw, handheld_pitch = handheld_offset(seconds, shot["handheld"])
        # 구도 오프셋만큼 조준을 비켜 대상을 화면 중앙에서 떼어 놓는다.
        yaw -= yaw_compose + handheld_yaw
        pitch -= pitch_compose + handheld_pitch

        # 주시 각이 ±180 을 넘어가면 부호가 뒤집혀 카메라가 한 바퀴 역회전한다.
        if previous_yaw is not None:
            while yaw - previous_yaw > 180.0:
                yaw -= 360.0
            while yaw - previous_yaw < -180.0:
                yaw += 360.0
        previous_yaw = yaw

        add_transform_keys(transform_section, frame, location, yaw, pitch)
        focus_distance_keys.append((frame, distance_between(location, focus_point)))

    # 초점은 대상 거리를 따라간다. 기본값(1km)을 두면 피사체가 통째로 흐려진다.
    add_float_track(
        component_binding, "FocusSettings.ManualFocusDistance", end_frame, focus_distance_keys)

    cut_track = sequence.add_track(unreal.MovieSceneCameraCutTrack)
    cut_section = cut_track.add_section()
    cut_section.set_range(0, end_frame)
    cut_section.set_camera_binding_id(sequence.get_binding_id(camera_binding))

    unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    return sequence, end_frame


def build_master(spec, shots, overwrite_existing):
    """샷을 순서대로 이어 붙인 마스터. 길이/순서 편집은 여기서만 한다."""
    master = create_sequence(spec["folder"], spec["master_name"], overwrite_existing)

    shot_track = master.add_track(unreal.MovieSceneCinematicShotTrack)
    cursor = 0
    boundaries = []
    for shot_sequence, shot_length in shots:
        section = shot_track.add_section()
        section.set_sequence(shot_sequence)
        section.set_range(cursor, cursor + shot_length)
        boundaries.append((cursor, shot_sequence.get_name()))
        cursor += shot_length

    master.set_playback_start_seconds(0.0)
    master.set_playback_end(cursor)

    # 진입/이탈 페이드 — 게임플레이에서 컷으로 튀어 들어가지 않게 한다.
    fade_frames = int(round(FADE_SECONDS * DISPLAY_RATE.numerator / DISPLAY_RATE.denominator))
    fade_track = master.add_track(unreal.MovieSceneFadeTrack)
    fade_section = fade_track.add_section()
    fade_section.set_range(0, cursor)
    fade_channel = fade_section.get_channels_by_type(unreal.MovieSceneScriptingFloatChannel)[0]
    for frame, value in (
        (0, 1.0), (fade_frames, 0.0), (cursor - fade_frames, 0.0), (cursor, 1.0),
    ):
        fade_channel.add_key(
            unreal.FrameNumber(frame), value,
            interpolation=unreal.MovieSceneKeyInterpolation.LINEAR)

    # 샷 경계를 마크로 남겨 편집 시 컷 지점을 눈으로 찾을 수 있게 한다.
    for frame, label in boundaries:
        master.add_marked_frame(
            unreal.MovieSceneMarkedFrame(unreal.FrameNumber(frame), label))

    unreal.EditorAssetLibrary.save_loaded_asset(master)
    return master


def build_cinematic(spec, overwrite_existing):
    focus_actor = require_focus_actor(spec["focus_actor_class"])
    shots = [
        build_shot(
            spec["folder"], shot, focus_actor, spec["shot_prefix"], overwrite_existing)
        for shot in spec["shots"]
    ]
    master = build_master(spec, shots, overwrite_existing)
    return {
        "master": master.get_path_name(),
        "shots": [sequence.get_path_name() for sequence, _ in shots],
    }


def run(master_names=None, overwrite_existing=False):
    """선택한 컷신만 생성한다. 기존 에셋 덮어쓰기는 호출자가 명시해야 한다."""
    selected_names = set(master_names or [spec["master_name"] for spec in CINEMATIC_SPECS])
    created = []
    for spec in CINEMATIC_SPECS:
        if spec["master_name"] not in selected_names:
            continue
        unreal.EditorAssetLibrary.make_directory(spec["folder"])
        unreal.EditorAssetLibrary.make_directory(f"{spec['folder']}/Shots")
        created.append(build_cinematic(spec, overwrite_existing))
    for entry in created:
        unreal.log(f"컷신 생성 완료: {entry['master']}")
    return created


if __name__ == "__main__":
    run()
