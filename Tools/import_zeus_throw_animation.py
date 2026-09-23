"""Import Mixamo's selected Throw clip for Zeus and register it for the upper-body slot."""
from pathlib import Path
import unreal

source = Path.home() / "Downloads" / "Throw.fbx"
if not source.is_file():
    raise RuntimeError(f"Download the selected Mixamo Throw animation to {source} first")

lib = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
skeleton = unreal.load_asset("/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig_Skeleton")
if not skeleton:
    raise RuntimeError("Could not load the Zeus Mixamo skeleton")

destination = "/Game/Thunderlord/Zeus/Animations/Mixamo"
asset_name = "MX_Throw_BasePose"
asset_path = f"{destination}/{asset_name}"
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
task.set_editor_property("filename", str(source))
task.set_editor_property("destination_path", destination)
task.set_editor_property("destination_name", asset_name)
task.set_editor_property("replace_existing", lib.does_asset_exist(asset_path))
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
asset_tools.import_asset_tasks([task])

clip = lib.load_asset(asset_path)
if not isinstance(clip, unreal.AnimSequence):
    raise RuntimeError(f"Mixamo animation import failed: {task.get_editor_property('imported_object_paths')}")
if clip.get_editor_property("skeleton") != skeleton:
    raise RuntimeError("Imported throw clip is not assigned to the Zeus Mixamo skeleton")

data = lib.load_asset("/Game/Thunderlord/Zeus/AnimationMap/DA_ThunderlordAnimations")
anim_bp = lib.load_asset("/Game/Thunderlord/Zeus/AnimationMap/ABP_Thunderlord")
standing = lib.load_asset("/Game/Thunderlord/Zeus/AnimationMap/BS_Thunderlord_WalkSprint")
crouch = lib.load_asset("/Game/Thunderlord/Zeus/AnimationMap/BS_Thunderlord_Crouch")
if not all((data, anim_bp, standing, crouch)):
    raise RuntimeError("A Thunderlord animation-map asset is missing")

# Rebuild from the authoritative in-place clips directly. Older versions of
# this asset have action map key names that differ from the current C++ API.
slide = lib.load_asset("/Game/Thunderlord/Zeus/Animations/Mixamo/MX_Slide_InPlace")
jump = lib.load_asset("/Game/Thunderlord/Zeus/Animations/Mixamo/MX_Jump_InPlace")
fall = lib.load_asset("/Game/Thunderlord/Zeus/Animations/Mixamo/MX_Fall_InPlace")
if not all((slide, jump, fall)):
    raise RuntimeError("One or more in-place movement clips are missing")

actions = dict(data.get_editor_property("actions"))
actions["Throw"] = clip
data.set_editor_property("actions", actions)
if not unreal.ThunderlordContentLibrary.configure_animation_blueprint(
        anim_bp, standing, crouch, slide, jump, fall):
    raise RuntimeError("Could not rebuild the layered upper-body animation graph")

anim_cdo = unreal.get_default_object(anim_bp.generated_class())
anim_cdo.set_editor_property("throw_release_fraction", 0.30)
anim_cdo.set_editor_property("throw_play_rate", 1.5)
lib.save_loaded_asset(data)
lib.save_loaded_asset(anim_bp)
unreal.log_warning(
    f"ZEUS_THROW_READY clip={clip.get_path_name()} skeleton={skeleton.get_path_name()} "
    f"slot=UpperBodyThrow release_fraction={anim_cdo.get_editor_property('throw_release_fraction')} "
    f"play_rate={anim_cdo.get_editor_property('throw_play_rate')}")
