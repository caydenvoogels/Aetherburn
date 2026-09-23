"""Import the downloaded no-skin Mixamo animation FBXs onto the Zeus skeleton."""
from pathlib import Path
import unreal


SOURCE = Path.home() / "Downloads" / "Aetherburn_Mixamo_Animations"
DESTINATION = "/Game/Thunderlord/Zeus/Animations/Mixamo"
SKELETON_PATH = "/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig_Skeleton.SKM_ZeusMaxiamoRig_Skeleton"

CLIPS = [
    "MX_Idle",
    "MX_Walk_Forward",
    "MX_Walk_Backward",
    "MX_Walk_Left",
    "MX_Walk_Right",
    "MX_Run_Forward",
    "MX_Run_Backward",
    "MX_Run_Left",
    "MX_Run_Right",
    "MX_Crouch_Idle",
    "MX_Crouch_Forward",
    "MX_Crouch_Backward",
    "MX_Crouch_Left",
    "MX_Crouch_Right",
    "MX_Jump",
    "MX_Fall",
    "MX_Land",
    "MX_Slide",
    "MX_Slide_Exit",
]

library = unreal.EditorAssetLibrary
skeleton = library.load_asset(SKELETON_PATH)
if not skeleton:
    raise RuntimeError(f"Zeus Mixamo skeleton is missing: {SKELETON_PATH}")
if not SOURCE.is_dir():
    raise RuntimeError(f"Animation FBX folder is missing: {SOURCE}")

library.make_directory(DESTINATION)
for clip_name in CLIPS:
    source_file = SOURCE / f"{clip_name}.fbx"
    destination_asset = f"{DESTINATION}/{clip_name}"
    if not source_file.is_file():
        raise RuntimeError(f"Animation FBX is missing: {source_file}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(
    world, "Interchange.FeatureFlags.Import.FBX 0"
)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
imported = []
for clip_name in CLIPS:
    source_file = SOURCE / f"{clip_name}.fbx"
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", clip_name)
    # The locomotion FBXs are re-exported from Mixamo with In Place enabled.
    # Reimport them in place so the existing data asset references stay valid.
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("factory", unreal.FbxFactory())
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    asset_path = f"{DESTINATION}/{clip_name}.{clip_name}"
    animation = library.load_asset(asset_path)
    if not animation or not isinstance(animation, unreal.AnimSequence):
        raise RuntimeError(
            f"Animation import failed for {source_file}; "
            f"imported={task.get_editor_property('imported_object_paths')}"
        )
    if animation.get_editor_property("skeleton") != skeleton:
        raise RuntimeError(f"Imported animation uses the wrong skeleton: {asset_path}")
    if animation.get_play_length() <= 0:
        raise RuntimeError(f"Imported animation has no duration: {asset_path}")
    library.save_loaded_asset(animation)
    imported.append(f"{asset_path} ({animation.get_play_length():.2f}s)")

library.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log_warning("MIXAMO_ANIMATION_IMPORT_OK " + " | ".join(imported))
