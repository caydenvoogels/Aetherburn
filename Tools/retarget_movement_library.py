"""Retarget CC0 movement clips through explicit IK assets; never edit binary files."""
import unreal

TARGET = '/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig'
OUT = '/Game/Thunderlord/Zeus/Movement'
assets = unreal.AssetToolsHelpers.get_asset_tools()

def create(path, cls, factory):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = path.rsplit('/', 1)
    return assets.create_asset(name, folder, cls, factory())

target = unreal.load_asset(TARGET)
target_rig = unreal.load_asset('/Game/Thunderlord/Zeus/Rigs/IK_Zeus')

for library in ['UAL1', 'UAL2']:
    folder = '/Game/ThirdParty/Quaternius/' + library
    all_assets = [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False)]
    if library == 'UAL2':
        all_assets += [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets('/Game/ThirdParty/Quaternius/SlideAnimations', recursive=True, include_folder=False)]
    meshes = [a for a in all_assets if isinstance(a, unreal.SkeletalMesh)]
    if not meshes:
        raise RuntimeError('Missing source mesh ' + folder)
    source = meshes[0]
    rig = create('/Game/Thunderlord/Zeus/Rigs/IK_' + library + '_Zeus', unreal.IKRigDefinition, unreal.IKRigDefinitionFactory)
    rc = unreal.IKRigController.get_controller(rig)
    rc.set_skeletal_mesh(source)
    rc.apply_auto_generated_retarget_definition()
    rc.set_retarget_root('pelvis')
    rtg = create('/Game/Thunderlord/Zeus/Rigs/RTG_' + library + '_Zeus', unreal.IKRetargeter, unreal.IKRetargetFactory)
    ctl = unreal.IKRetargeterController.get_controller(rtg)
    ctl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig)
    ctl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    ctl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source)
    ctl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target)
    ctl.remove_all_ops()
    ctl.add_default_ops()
    ctl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    # Preserve the imported Mixamo reference pose; full target auto-alignment
    # previously pulled the hips out of place in animation previews.
    wanted = ['Crouch_Fwd_Loop', 'Crouch_Idle_Loop', 'Sprint_Loop', 'Idle_Loop', 'Jump_Start', 'Jump_Loop', 'Jump_Land'] if library == 'UAL1' else ['Slide_Start', 'Slide_Loop', 'Slide_Exit']
    inputs = unreal.IKRetargetBatchOperationInputs()
    clips = [a for a in all_assets if isinstance(a, unreal.AnimSequence) and any(a.get_name().endswith(w) for w in wanted)]
    inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(a.get_path_name()) for a in clips]
    inputs.source_mesh = source
    inputs.target_mesh = target
    inputs.ik_retarget_asset = rtg
    inputs.prefix = 'TL_'
    inputs.target_path = OUT
    inputs.include_referenced_assets = False
    inputs.overwrite_existing_files = True
    generated = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    unreal.EditorAssetLibrary.save_directory('/Game/Thunderlord/Zeus', only_if_is_dirty=False, recursive=True)
    if not generated:
        raise RuntimeError('No retargeted clips for ' + library)
    unreal.log_warning(f'RETARGETED_{library} count={len(generated)}')

unreal.EditorAssetLibrary.save_directory('/Game/Thunderlord/Zeus', only_if_is_dirty=False, recursive=True)
for path in unreal.EditorAssetLibrary.list_assets(OUT, recursive=True, include_folder=False):
    clip = unreal.load_asset(path)
    if isinstance(clip, unreal.AnimSequence):
        assert clip.get_editor_property('skeleton') == target.get_editor_property('skeleton'), path
        unreal.log_warning(f'MOVEMENT_CLIP {path} duration={clip.get_play_length()}')
