import os
import unreal

ROOT = r'C:\Users\cvoog\Downloads\KayKit_Character_Animations_1.1\KayKit_Character_Animations_1.1'
DEST = '/Game/ThirdParty/KayKit'

def run_import(filename, destination, import_mesh, skeleton=None):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if import_mesh else unreal.FBXImportType.FBXIT_ANIMATION
    options.import_as_skeletal = True
    options.import_mesh = import_mesh
    options.import_animations = True
    options.import_materials = False
    options.import_textures = False
    options.skeleton = skeleton
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.log_warning(f'KAYKIT_IMPORT file={filename} assets={task.imported_object_paths}')
    return task.imported_object_paths

character = os.path.join(ROOT, 'Mannequin Character', 'characters', 'Mannequin_Medium.fbx')
created = run_import(character, DEST + '/Source', True)

skeletons = [unreal.load_asset(path) for path in created if path.endswith('_Skeleton')]
if not skeletons:
    skeletons = unreal.EditorAssetLibrary.list_assets(DEST + '/Source', recursive=True, include_folder=False)
    skeletons = [unreal.load_asset(path) for path in skeletons if isinstance(unreal.load_asset(path), unreal.Skeleton)]
if not skeletons:
    raise RuntimeError('KayKit source skeleton was not created')
skeleton = skeletons[0]

anim_root = os.path.join(ROOT, 'Animations', 'fbx', 'Rig_Medium')
for name in ['Rig_Medium_MovementBasic.fbx', 'Rig_Medium_MovementAdvanced.fbx']:
    run_import(os.path.join(anim_root, name), DEST + '/SourceAnimations', False, skeleton)

unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
