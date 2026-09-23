from pathlib import Path
import unreal

root = Path('C:/Users/cvoog/Downloads/QuaterniusUAL2')
source = next(root.rglob('UAL2_Standard.fbx'))
task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = '/Game/ThirdParty/Quaternius/UAL2'
task.automated = True
task.save = True
task.replace_existing = True
task.factory = unreal.FbxFactory()
options = unreal.FbxImportUI()
options.automated_import_should_detect_type = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.import_as_skeletal = True
options.import_mesh = True
options.import_animations = True
options.import_materials = False
options.import_textures = False
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assets = unreal.EditorAssetLibrary.list_assets(task.destination_path, recursive=True, include_folder=False)
unreal.EditorAssetLibrary.save_directory(task.destination_path, only_if_is_dirty=False, recursive=True)
for path in assets:
    obj = unreal.load_asset(path)
    if isinstance(obj, unreal.AnimSequence) and any(word in path for word in ['Slide', 'Idle']):
        unreal.log_warning(f'UAL_ANIMATION {path} length={obj.get_play_length()}')
    elif isinstance(obj, unreal.SkeletalMesh):
        unreal.log_warning(f'UAL_MESH {path}')
unreal.log_warning(f'UAL_IMPORTED count={len(assets)}')

source_one = next(Path('C:/Users/cvoog/Downloads/QuaterniusUAL1').rglob('UAL1_Standard.fbx'))
task.filename = str(source_one)
task.destination_path = '/Game/ThirdParty/Quaternius/UAL1'
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.EditorAssetLibrary.save_directory(task.destination_path, only_if_is_dirty=False, recursive=True)
unreal.log_warning('UAL1_IMPORTED')
