from pathlib import Path
import unreal

task = unreal.AssetImportTask()
task.filename = str(next(Path('C:/Users/cvoog/Downloads/QuaterniusUAL2').rglob('UAL2_Standard.fbx')))
task.destination_path = '/Game/ThirdParty/Quaternius/SlideAnimations'
task.destination_name = 'SlideLibrary'
task.automated = True
task.save = True
task.replace_existing = True
task.factory = unreal.FbxFactory()
options = unreal.FbxImportUI()
options.automated_import_should_detect_type = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
options.import_as_skeletal = True
options.import_mesh = False
options.import_animations = True
options.skeleton = unreal.load_asset('/Game/ThirdParty/Quaternius/UAL2/UAL2_Standard_Skeleton')
task.options = options
unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), 'Interchange.FeatureFlags.Import.FBX 0')
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.log_warning(f'SLIDE_IMPORTED_OBJECTS {task.imported_object_paths}')
unreal.EditorAssetLibrary.save_directory(task.destination_path, only_if_is_dirty=False, recursive=True)
unreal.log_warning('SLIDE_IMPORT_SAVED')
