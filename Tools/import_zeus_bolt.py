"""Import the Zeus bolt FBX as a static mesh, including embedded images."""
from pathlib import Path
import unreal

SOURCE = Path.home() / "Downloads" / "ZeusBolt.fbx"
DESTINATION = "/Game/Thunderlord/Zeus/Weapons"
ASSET_NAME = "SM_ZeusBolt"

if not SOURCE.is_file():
    raise RuntimeError(f"FBX not found: {SOURCE}")

library = unreal.EditorAssetLibrary
asset_path = f"{DESTINATION}/{ASSET_NAME}.{ASSET_NAME}"
task = None
options = unreal.FbxImportUI()
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
options.set_editor_property("import_as_skeletal", False)
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)

task = unreal.AssetImportTask()
task.set_editor_property("filename", str(SOURCE))
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", ASSET_NAME)
task.set_editor_property("replace_existing", library.does_asset_exist(asset_path))
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = library.load_asset(asset_path)
if not mesh or not isinstance(mesh, unreal.StaticMesh):
    raise RuntimeError(f"Static Mesh import failed; imported={task.get_editor_property('imported_object_paths')}")

library.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
imported_assets = []
for path in library.list_assets(DESTINATION, recursive=True, include_folder=False):
    asset = library.load_asset(path)
    if asset:
        imported_assets.append(f"{path} [{asset.get_class().get_name()}]")
        if isinstance(asset, unreal.MaterialInterface):
            referencers = library.find_package_referencers_for_asset(path, load_assets_to_confirm=True)
            unreal.log_warning(f"ZEUS_BOLT_MATERIAL path={path} referencers={referencers}")
            try:
                texture_parameters = asset.get_editor_property("texture_parameter_values")
                texture_paths = []
                for parameter in texture_parameters:
                    texture = parameter.get_editor_property("parameter_value")
                    texture_paths.append(texture.get_path_name() if texture else None)
                unreal.log_warning(f"ZEUS_BOLT_MATERIAL_TEXTURES path={path} textures={texture_paths}")
            except Exception as error:
                unreal.log_warning(f"ZEUS_BOLT_MATERIAL_TEXTURES_UNAVAILABLE path={path} reason={error}")

texture_assets = [path for path in library.list_assets(DESTINATION, recursive=True, include_folder=False)
                  if isinstance(library.load_asset(path), unreal.Texture2D)]
if not texture_assets:
    unreal.log_warning("ZEUS_BOLT_NO_SEPARATE_TEXTURE_ASSETS: the FBX importer left image data in the source file")
unreal.log_warning(
    f"ZEUS_BOLT_IMPORT_OK asset={mesh.get_path_name()} bounds={mesh.get_bounds()} "
    f"textures={texture_assets} assets={imported_assets} "
    f"imported={task.get_editor_property('imported_object_paths') if task else 'already_imported'}"
)
