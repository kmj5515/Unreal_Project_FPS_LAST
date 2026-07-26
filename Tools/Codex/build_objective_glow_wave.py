import unreal


MATERIAL_PATH = "/Game/Assets/Art/Materials/M_UI_ObjectiveGlow"
TEXTURE_DESTINATION = "/Game/Assets/Art/UI/Texture/icon"
TEXTURE_NAME = "UI_Mask_Diamond"
TEXTURE_PATH = f"{TEXTURE_DESTINATION}/{TEXTURE_NAME}"
TEXTURE_SOURCE = (
    r"C:\Users\hope\Desktop\UI\Materials\Textures\UI_Mask_Diamond.png"
)


def require_asset(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        asset_name = asset_path.rsplit("/", 1)[-1]
        asset = unreal.load_object(None, f"{asset_path}.{asset_name}")
    if not asset:
        raise RuntimeError(f"에셋을 불러오지 못했습니다: {asset_path}")
    return asset


def import_mask_texture():
    texture = unreal.EditorAssetLibrary.load_asset(TEXTURE_PATH)
    if not texture:
        texture = unreal.load_object(
            None, f"{TEXTURE_PATH}.{TEXTURE_NAME}"
        )
    if not texture:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", TEXTURE_SOURCE)
        task.set_editor_property("destination_path", TEXTURE_DESTINATION)
        task.set_editor_property("destination_name", TEXTURE_NAME)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)

        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = require_asset(TEXTURE_PATH)

    # 파장 UV가 텍스처 경계를 넘을 때 반대편 무늬가 반복되지 않도록 제한한다.
    texture.set_editor_property("srgb", False)
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_MASKS
    )
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    texture.modify()

    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, False):
        # 실행 중 생성된 에셋이 레지스트리에 늦게 반영되는 경우가 있어 머티리얼 적용은 계속한다.
        unreal.log_warning(
            f"[Codex] 텍스처 레지스트리 저장을 건너뜁니다: {TEXTURE_PATH}"
        )
    return texture


def create_expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def create_scalar(material, name, value, x, y):
    expression = create_expression(
        material, unreal.MaterialExpressionScalarParameter, x, y
    )
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("default_value", value)
    return expression


def connect(source, source_output, target, target_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, source_output, target, target_input
    ):
        raise RuntimeError(
            f"노드 연결에 실패했습니다: {source.get_name()}.{source_output}"
            f" -> {target.get_name()}.{target_input}"
        )


def build_material(texture):
    material = require_asset(MATERIAL_PATH)
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    # UE 5.7의 레거시 표현식과 EditorOnly 표현식 컬렉션을 모두 정리한다.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)

    time = create_expression(material, unreal.MaterialExpressionTime, -1500, -500)
    wave_speed = create_scalar(material, "WaveSpeed", 0.8, -1500, -400)
    phase_multiply = create_expression(
        material, unreal.MaterialExpressionMultiply, -1250, -470
    )
    phase = create_expression(material, unreal.MaterialExpressionFrac, -1050, -470)
    connect(time, "", phase_multiply, "A")
    connect(wave_speed, "", phase_multiply, "B")
    connect(phase_multiply, "", phase, "")

    start_scale = create_scalar(material, "StartScale", 0.45, -1050, -180)
    end_scale = create_scalar(material, "EndScale", 1.0, -1050, -80)
    wave_scale = create_expression(
        material, unreal.MaterialExpressionLinearInterpolate, -800, -150
    )
    connect(start_scale, "", wave_scale, "A")
    connect(end_scale, "", wave_scale, "B")
    connect(phase, "", wave_scale, "Alpha")

    tex_coord = create_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -1050, 130
    )
    center_a = create_expression(
        material, unreal.MaterialExpressionConstant2Vector, -1050, 240
    )
    center_a.set_editor_property("r", 0.5)
    center_a.set_editor_property("g", 0.5)

    centered_uv = create_expression(
        material, unreal.MaterialExpressionSubtract, -800, 160
    )
    scaled_uv = create_expression(
        material, unreal.MaterialExpressionDivide, -580, 150
    )
    center_b = create_expression(
        material, unreal.MaterialExpressionConstant2Vector, -580, 280
    )
    center_b.set_editor_property("r", 0.5)
    center_b.set_editor_property("g", 0.5)
    final_uv = create_expression(material, unreal.MaterialExpressionAdd, -360, 170)

    connect(tex_coord, "", centered_uv, "A")
    connect(center_a, "", centered_uv, "B")
    connect(centered_uv, "", scaled_uv, "A")
    connect(wave_scale, "", scaled_uv, "B")
    connect(scaled_uv, "", final_uv, "A")
    connect(center_b, "", final_uv, "B")

    texture_sample = create_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -100, 100
    )
    texture_sample.set_editor_property("parameter_name", "GlowTexture")
    texture_sample.set_editor_property("texture", texture)
    connect(final_uv, "", texture_sample, "UVs")

    one_minus = create_expression(
        material, unreal.MaterialExpressionOneMinus, -800, -500
    )
    fade_power = create_scalar(material, "FadePower", 1.5, -800, -390)
    fade = create_expression(material, unreal.MaterialExpressionPower, -560, -470)
    connect(phase, "", one_minus, "")
    connect(one_minus, "", fade, "Base")
    connect(fade_power, "", fade, "Exp")

    wave_mask = create_expression(
        material, unreal.MaterialExpressionMultiply, 160, -10
    )
    # 이 DDS 원본은 Alpha가 상수 1이므로 실제 마스크가 저장된 R 채널을 사용한다.
    connect(texture_sample, "R", wave_mask, "A")
    connect(fade, "", wave_mask, "B")

    glow_color = create_expression(
        material, unreal.MaterialExpressionVectorParameter, 150, -280
    )
    glow_color.set_editor_property("parameter_name", "GlowColor")
    glow_color.set_editor_property(
        "default_value", unreal.LinearColor(0.061864, 1.0, 0.438796, 1.0)
    )
    colored_wave = create_expression(
        material, unreal.MaterialExpressionMultiply, 420, -180
    )
    connect(glow_color, "", colored_wave, "A")
    connect(wave_mask, "", colored_wave, "B")

    wave_intensity = create_scalar(material, "WaveIntensity", 2.0, 420, -330)
    final_color = create_expression(
        material, unreal.MaterialExpressionMultiply, 680, -180
    )
    connect(colored_wave, "", final_color, "A")
    connect(wave_intensity, "", final_color, "B")

    wave_opacity = create_scalar(material, "WaveOpacity", 0.8, 420, 80)
    final_opacity = create_expression(
        material, unreal.MaterialExpressionMultiply, 680, 20
    )
    connect(wave_mask, "", final_opacity, "A")
    connect(wave_opacity, "", final_opacity, "B")

    if not unreal.MaterialEditingLibrary.connect_material_property(
        final_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("Final Color 연결에 실패했습니다.")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        final_opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Opacity 연결에 실패했습니다.")

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError(f"머티리얼 저장에 실패했습니다: {MATERIAL_PATH}")

    expression_count = unreal.MaterialEditingLibrary.get_num_material_expressions(
        material
    )
    unreal.log(
        f"[Codex] Objective Glow 파장 머티리얼 저장 완료: "
        f"{MATERIAL_PATH}, Expressions={expression_count}"
    )


texture_asset = import_mask_texture()
build_material(texture_asset)
