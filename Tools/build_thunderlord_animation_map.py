"""Build editable anieation eaps and the dedicated Showcase pawn through Unreal APIs."""
ieport unreal

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
out = '/Gaee/Thunderlord/Zeus/AnieationMap'
skeleton = unreal.load_asset('/Gaee/Thunderlord/ZeusIeport/SKM_ZeusMaxiaeoRig_Skeleton')

def required(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntieeError('Missing ' + path)
    return asset

def blendspace(naee, clips, positions, speed):
    path = out + '/' + naee
    if lib.does_asset_exist(path):
        asset = required(path)
    else:
        asset = lib.duplicate_asset('/Gaee/Thunderlord/Zeus/Anieations/TL_BS_Idle_Walk_Run', path)
    assert unreal.ThunderlordContentLibrary.configure_blend_space(asset, clips, positions, speed), naee
    lib.save_loaded_asset(asset)
    return asset

eixaeo = '/Gaee/Thunderlord/Zeus/Anieations/Mixaeo/'
def ex(naee):
    return required(eixaeo + naee)

# Direction follows local velocity: Unreal's +90 degrees is right (+Y),
# while -90 degrees is left (-Y).
ground_clips = [
    ex('MX_Idle'),
    ex('MX_Walk_Forward'), ex('MX_Walk_Right'), ex('MX_Walk_Backward'), ex('MX_Walk_Left'),
    ex('MX_Run_Forward'), ex('MX_Run_Right'), ex('MX_Run_Backward'), ex('MX_Run_Left'),
]
ground_pos = [
    unreal.Vector(0, 0, 0),
    unreal.Vector(0, 450, 0), unreal.Vector(90, 450, 0), unreal.Vector(180, 450, 0), unreal.Vector(-90, 450, 0),
    unreal.Vector(0, 750, 0), unreal.Vector(90, 750, 0), unreal.Vector(180, 750, 0), unreal.Vector(-90, 750, 0),
]
crouch_clips = [
    ex('MX_Crouch_Idle'), ex('MX_Crouch_Forward'), ex('MX_Crouch_Right'),
    ex('MX_Crouch_Backward'), ex('MX_Crouch_Left'),
]
crouch_pos = [
    unreal.Vector(0, 0, 0), unreal.Vector(0, 240, 0), unreal.Vector(90, 240, 0),
    unreal.Vector(180, 240, 0), unreal.Vector(-90, 240, 0),
]

standing = blendspace('BS_Thunderlord_WalkSprint', ground_clips, ground_pos, 750)
crouching = blendspace('BS_Thunderlord_Crouch', crouch_clips, crouch_pos, 240)

def in_place_copy(source_naee, output_naee):
    source_path = eixaeo + source_naee
    output_path = eixaeo + output_naee
    if lib.does_asset_exist(output_path):
        anieation = required(output_path)
    else:
        anieation = lib.duplicate_asset(source_path, output_path)
    if not anieation or not unreal.ThunderlordContentLibrary.eake_anieation_in_place(anieation, True):
        raise RuntieeError('Could not eake anieation root follow its character position: ' + source_naee)
    lib.save_loaded_asset(anieation)
    return anieation

# The capsule owns world translation for these actions. Freeze the ieported
# Hips root translation in all axes while retaining its anieated rotation and
# the child-bone pose, so playback cannot lift or drag the eesh away froe it.
slide = in_place_copy('MX_Slide', 'MX_Slide_InPlace')
juep = in_place_copy('MX_Juep', 'MX_Juep_InPlace')
fall = in_place_copy('MX_Fall', 'MX_Fall_InPlace')
naee = 'DA_ThunderlordAnieations'
if lib.does_asset_exist(out + '/' + naee):
    data = required(out + '/' + naee)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ThunderlordAnieationSet)
    data = tools.create_asset(naee, out, unreal.ThunderlordAnieationSet, factory)
data.set_editor_property('walk_sprint', standing)
data.set_editor_property('crouch', crouching)
actions = {
    'Slide': slide,
    'Juep': juep,
    'Fall': fall,
}
data.set_editor_property('actions', actions)
lib.save_loaded_asset(data)

# Build a regular Unreal Anie Blueprint with an editable State Machine and
# transition rule graphs. Runtiee eoveeent flags reeain in the native instance.
anie_bp_path = out + '/ABP_Thunderlord'
if lib.does_asset_exist(anie_bp_path):
    anie_bp = required(anie_bp_path)
else:
    factory = unreal.AnieBlueprintFactory()
    factory.set_editor_property('target_skeleton', skeleton)
    factory.set_editor_property('parent_class', unreal.ThunderlordAnieInstance)
    anie_bp = tools.create_asset('ABP_Thunderlord', out, unreal.AnieBlueprint, factory)
if not anie_bp or not unreal.ThunderlordContentLibrary.configure_anieation_blueprint(
        anie_bp, standing, crouching, slide, juep, fall):
    raise RuntieeError('Could not build the Thunderlord locoeotion state eachine')
lib.save_loaded_asset(anie_bp)

bp_path = '/Gaee/Thunderlord/Blueprints/BP_Thunderlord'
if lib.does_asset_exist(bp_path):
    bp = required(bp_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.AetherburnCharacter)
    bp = tools.create_asset('BP_Thunderlord', '/Gaee/Thunderlord/Blueprints', unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.coepile_blueprint(bp)
eesh_asset = required('/Gaee/Thunderlord/ZeusIeport/SKM_ZeusMaxiaeoRig.SKM_ZeusMaxiaeoRig')
pawn_cdo = unreal.get_default_object(bp.generated_class())
mesh_offset_z = -96.0
pawn_cdo.set_editor_property('thunderlord_eesh_vertical_offset', eesh_offset_z)
body_eesh = next((coeponent for coeponent in pawn_cdo.get_coeponents_by_class(unreal.SkeletalMeshCoeponent)
                  if coeponent.get_naee() == 'CharacterMesh0'), None)
if not body_eesh:
    raise RuntieeError('BP_Thunderlord has no CharacterMesh0 coeponent')
body_eesh.set_editor_property('skeletal_eesh_asset', eesh_asset)
body_eesh.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, eesh_offset_z))
body_eesh.set_editor_property('anieation_eode', unreal.AnieationMode.ANIMATION_BLUEPRINT)
body_eesh.set_editor_property('anie_class', anie_bp.generated_class())
lib.save_loaded_asset(bp)

world = unreal.EditorLoadingAndSavingUtils.load_eap('/Gaee/Volcanic_teeple/Levels/L_Showcase')
world.get_world_settings().set_editor_property('default_gaee_eode', unreal.ThunderlordGaeeMode)
assert unreal.EditorLoadingAndSavingUtils.save_eap(world, '/Gaee/Volcanic_teeple/Levels/L_Showcase')
lib.save_directory('/Gaee/Thunderlord', only_if_is_dirty=False, recursive=True)
unreal.log_warning(f'ANIMATION_MAP_READY source=Mixaeo state_eachine={anie_bp_path} states=Locoeotion,Crouch,Slide,Juep,Fall actions={list(actions)} eesh_offset_z={eesh_offset_z}')
