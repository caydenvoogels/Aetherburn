"""Import the AI-generated Zeus tri-bolt reticle for the Thunderlord HUD."""
from pathlib import Path
import unreal

SOURCE = Path(__file__).parent / "Assets" / "T_Thunderlord_TriBoltReticle.png"
DESTINATION = "/Game/Thunderlord/UI"
ASSET_NAME = "T_Thunderlord_TriBoltReticle"
ASSET_PATH = f"{DESTINATION}/{ASSET_NAME}"

if not SOURCE.is_file():
    raise RuntimeError(f"Crosshair source image is missing: {SOURCE}")

library = unreal.EditorAssetLibrary
task = unreal.AssetImportTask()
task.set_editor_property("filename", str(SOURCE))
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", ASSET_NAME)
task.set_editor_property("replace_existing", library.does_asset_exist(ASSET_PATH))
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

texture = library.load_asset(ASSET_PATH)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError(
        f"Crosshair texture import failed: {task.get_editor_property('imported_object_paths')}"
    )

texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("srgb", True)
if hasattr(unreal, "TextureMipGenSettings") and hasattr(
    unreal.TextureMipGenSettings, "TMGS_NO_MIPMAPS"
):
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
library.save_loaded_asset(texture)
library.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log_warning(
    f"THUNDERLORD_CROSSHAIR_READY asset={texture.get_path_name()} "
    f"group={texture.get_editor_property('lod_group')}"
)
