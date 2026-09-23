"""Build editable animation maps and the dedicated Showcase pawn through Unreal APIs."""
import unreal

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
out = '/Game/Thunderlord/Zeus/AnimationMap'
skeleton = unreal.load_asset('/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig_Skeleton')

def required(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError('Missing ' + path)
    return asset

def blendspace(name, clips, positions, speed):
    path = out + '/' + name
    if lib.does_asset_exist(path):
        asset = required(path)
    else:
        asset = lib.duplicate_asset('/Game/Thunderlord/Zeus/Animations/TL_BS_Idle_Walk_Run', path)
    assert unreal.ThunderlordContentLibrary.configure_blend_space(asset, clips, positions, speed), name
    lib.save_loaded_asset(asset)
    return asset

mixamo = '/Game/Thunderlord/Zeus/Animations/Mixamo/'
def mx(name):
    return required(mixamo + name)

# Direction follows local velocity: Unreal's +90 degrees is right (+Y),
# while -90 degrees is left (-Y).
ground_clips = [
    mx('MX_Idle'),
    mx('MX_Walk_Forward'), mx('MX_Walk_Right'), mx('MX_Walk_Backward'), mx('MX_Walk_Left'),
    mx('MX_Run_Forward'), mx('MX_Run_Right'), mx('MX_Run_Backward'), mx('MX_Run_Left'),
]
ground_pos = [
    unreal.Vector(0, 0, 0),
    unreal.Vector(0, 450, 0), unreal.Vector(90, 450, 0), unreal.Vector(180, 450, 0), unreal.Vector(-90, 450, 0),
    unreal.Vector(0, 750, 0), unreal.Vector(90, 750, 0), unreal.Vector(180, 750, 0), unreal.Vector(-90, 750, 0),
]
crouch_clips = [
    mx('MX_Crouch_Idle'), mx('MX_Crouch_Forward'), mx('MX_Crouch_Right'),
    mx('MX_Crouch_Backward'), mx('MX_Crouch_Left'),
]
crouch_pos = [
    unreal.Vector(0, 0, 0), unreal.Vector(0, 240, 0), unreal.Vector(90, 240, 0),
    unreal.Vector(180, 240, 0), unreal.Vector(-90, 240, 0),
]

standing = blendspace('BS_Thunderlord_WalkSprint', ground_clips, ground_pos, 750)
crouching = blendspace('BS_Thunderlord_Crouch', crouch_clips, crouch_pos, 240)

def in_place_copy(source_name, output_name):
    source_path = mixamo + source_name
    output_path = mixamo + output_name
    if lib.does_asset_exist(output_path):
        animation = required(output_path)
    else:
        animation = lib.duplicate_asset(source_path, output_path)
    if not animation or not unreal.ThunderlordContentLibrary.make_animation_in_place(animation, True):
        raise RuntimeError('Could not make animation root follow its character position: ' + source_name)
    lib.save_loaded_asset(animation)
    return animation

# The capsule owns world translation for these actions. Freeze the imported
# Hips root translation in all axes while retaining its animated rotation and
# the child-bone pose, so playback cannot lift or drag the mesh away from it.
slide = in_place_copy('MX_Slide', 'MX_Slide_InPlace')
jump = in_place_copy('MX_Jump', 'MX_Jump_InPlace')
fall = in_place_copy('MX_Fall', 'MX_Fall_InPlace')
name = 'DA_ThunderlordAnimations'
if lib.does_asset_exist(out + '/' + name):
    data = required(out + '/' + name)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ThunderlordAnimationSet)
    data = tools.create_asset(name, out, unreal.ThunderlordAnimationSet, factory)
data.set_editor_property('walk_sprint', standing)
data.set_editor_property('crouch', crouching)
actions = {
    'Slide': slide,
    'Jump': jump,
    'Fall': fall,
}
data.set_editor_property('actions', actions)
lib.save_loaded_asset(data)

# Build a regular Unreal Anim Blueprint with an editable State Machine and
# transition rule graphs. Runtime movement flags remain in the native instance.
anim_bp_path = out + '/ABP_Thunderlord'
if lib.does_asset_exist(anim_bp_path):
    anim_bp = required(anim_bp_path)
else:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property('target_skeleton', skeleton)
    factory.set_editor_property('parent_class', unreal.ThunderlordAnimInstance)
    anim_bp = tools.create_asset('ABP_Thunderlord', out, unreal.AnimBlueprint, factory)
if not anim_bp or not unreal.ThunderlordContentLibrary.configure_animation_blueprint(
        anim_bp, standing, crouching, slide, jump, fall):
    raise RuntimeError('Could not build the Thunderlord locomotion state machine')
lib.save_loaded_asset(anim_bp)

bp_path = '/Game/Thunderlord/Blueprints/BP_Thunderlord'
if lib.does_asset_exist(bp_path):
    bp = required(bp_path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.AetherburnCharacter)
    bp = tools.create_asset('BP_Thunderlord', '/Game/Thunderlord/Blueprints', unreal.Blueprint, factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
mesh_asset = required('/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig')
pawn_cdo = unreal.get_default_object(bp.generated_class())
mesh_offset_z = -60.0
pawn_cdo.set_editor_property('thunderlord_mesh_vertical_offset', mesh_offset_z)
body_mesh = next((component for component in pawn_cdo.get_components_by_class(unreal.SkeletalMeshComponent)
                  if component.get_name() == 'CharacterMesh0'), None)
if not body_mesh:
    raise RuntimeError('BP_Thunderlord has no CharacterMesh0 component')
body_mesh.set_editor_property('skeletal_mesh_asset', mesh_asset)
body_mesh.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, mesh_offset_z))
body_mesh.set_editor_property('animation_mode', unreal.AnimationMode.ANIMATION_BLUEPRINT)
body_mesh.set_editor_property('anim_class', anim_bp.generated_class())
lib.save_loaded_asset(bp)

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Volcanic_temple/Levels/L_Showcase')
world.get_world_settings().set_editor_property('default_game_mode', unreal.ThunderlordGameMode)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Volcanic_temple/Levels/L_Showcase')
lib.save_directory('/Game/Thunderlord', only_if_is_dirty=False, recursive=True)
unreal.log_warning(f'ANIMATION_MAP_READY source=Mixamo state_machine={anim_bp_path} states=Locomotion,Crouch,Slide,Jump,Fall actions={list(actions)} mesh_offset_z={mesh_offset_z}')
