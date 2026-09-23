"""Import ZeusMaxiamoRig as a skeletal mesh using UE's legacy FBX importer.

Unreal 5.8's experimental Interchange FBX importer fails to decode this file's
embedded texture payloads. Temporarily set Interchange.FeatureFlags.Import.FBX
to False in DefaultEngine.ini for the import commandlet, then restore the
project config. The import result must contain both embedded Texture2D assets.
"""
from pathlib import Path
import unreal

SOURCE = Path.home() / "Downloads" / "ZeusMaxiamoRig.fbx"
DESTINATION = "/Game/Thunderlord/ZeusImport"
ASSET_NAME = "SKM_ZeusMaxiamoRig"

if not SOURCE.is_file():
    raise RuntimeError(f"FBX not found: {SOURCE}")

library = unreal.EditorAssetLibrary
if library.does_asset_exist(f"{DESTINATION}/{ASSET_NAME}"):
    raise RuntimeError("Destination asset already exists; refusing to overwrite it")

options = unreal.FbxImportUI()
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)
options.set_editor_property("create_physics_asset", True)
options.set_editor_property("skeleton", None)

task = unreal.AssetImportTask()
task.set_editor_property("filename", str(SOURCE))
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", ASSET_NAME)
task.set_editor_property("replace_existing", False)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

paths = task.get_editor_property("imported_object_paths")
mesh_path = f"{DESTINATION}/{ASSET_NAME}.{ASSET_NAME}"
mesh = library.load_asset(mesh_path)
if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
    raise RuntimeError(f"Skeletal Mesh import failed; imported={paths}")

skeleton = mesh.get_editor_property("skeleton")
physics = mesh.get_editor_property("physics_asset")
assets = []
texture_count = 0
for path in library.list_assets(DESTINATION, recursive=True, include_folder=False):
    asset = library.load_asset(path)
    if asset:
        assets.append(f"{path} [{asset.get_class().get_name()}]")
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        if isinstance(asset, unreal.Texture2D):
            texture_count += 1

if texture_count == 0:
    raise RuntimeError(
        "Legacy FBX import produced no Texture2D assets; embedded textures were not extracted"
    )

unreal.log_warning(
    "ZEUS_IMPORT_OK "
    f"mesh={mesh.get_path_name()} skeleton={skeleton.get_path_name() if skeleton else None} "
    f"physics={physics.get_path_name() if physics else None} bounds={mesh.get_bounds()} "
    f"embedded_texture_count={texture_count} "
    f"assets={assets} imported={paths}"
)
