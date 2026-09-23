"""Validate Hyper footstep, affiliation, and outline setup in the Showcase."""
import unreal

MAP = '/Game/Volcanic_temple/Levels/L_Showcase'
PAWN_PATH = '/Game/Thunderlord/Blueprints/BP_Thunderlord'
CONTROLLER_PATH = '/Game/Thunderlord/Blueprints/BP_AetherburnPlayerController'
FOOTSTEP_PATH = '/Game/Hyper/FootstepSystem/Blueprints/AC_CH_Footsteps'
MOVEMENT_PATH = '/Game/Hyper/Locomotion/Blueprints/AC_CH_Extended_Movement_Base'
IDENTIFIER_PATH = '/Game/Hyper/Core/ActorIdentifier/Blueprints/AC_Actor_Identifier'
OUTLINER_PATH = '/Game/Hyper/Outliner_System/Blueprints/AC_PC_Outliner'
WORLD_OUTLINER_PATH = '/Game/Hyper/Outliner_System/Blueprints/BP_Outliner_World_Actor'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def gameplay_tag(name):
    tag = unreal.GameplayTag()
    require(tag.import_text('(TagName="' + name + '")'), 'Missing Gameplay Tag ' + name)
    return tag


world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
settings = world.get_world_settings()
game_mode = settings.get_editor_property('default_game_mode')
require(game_mode and game_mode.get_name() == 'ThunderlordGameMode', 'Showcase GameMode changed')
game_mode_cdo = unreal.get_default_object(game_mode)
player_pawn_class = game_mode_cdo.get_editor_property('default_pawn_class')
controller_class = game_mode_cdo.get_editor_property('player_controller_class')
require(player_pawn_class and player_pawn_class.get_name() == 'BP_Thunderlord_C', 'Unexpected player pawn')
require(controller_class and controller_class.get_name() == 'BP_AetherburnPlayerController_C', 'Unexpected player controller')

blue = gameplay_tag('Team Affiliation.Blue')
red = gameplay_tag('Team Affiliation.Red')
npc_id = gameplay_tag('NPC.ID')
footstep_class = unreal.load_class(None, FOOTSTEP_PATH + '.AC_CH_Footsteps_C')
movement_class = unreal.load_class(None, MOVEMENT_PATH + '.AC_CH_Extended_Movement_Base_C')
identifier_class = unreal.load_class(None, IDENTIFIER_PATH + '.AC_Actor_Identifier_C')
require(footstep_class and movement_class and identifier_class, 'A required Hyper component class is missing')

pawn_bp = unreal.load_asset(PAWN_PATH)
pawn_cdo = unreal.get_default_object(pawn_bp.generated_class())
camera_booms = pawn_cdo.get_components_by_class(unreal.SpringArmComponent)
require(bool(camera_booms), 'BP_Thunderlord has no camera boom')
camera_boom = camera_booms[0]
require(abs(camera_boom.get_editor_property('target_arm_length')) < 0.01,
        'Showcase camera is not configured for first person')
require(camera_boom.get_editor_property('socket_offset').is_nearly_zero(),
        'First-person camera has a third-person socket offset')
mesh = next((item for item in pawn_cdo.get_components_by_class(unreal.SkeletalMeshComponent)
             if item.get_name() == 'CharacterMesh0'), None)
require(mesh is not None, 'BP_Thunderlord CharacterMesh0 is missing')
require(abs(mesh.get_editor_property('relative_location').z + 96.0) < 0.01,
        'Zeus mesh offset relative to capsule is not -96')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
subobject_library = unreal.SubobjectDataBlueprintFunctionLibrary
component_paths = set()
for handle in subsystem.k2_gather_subobject_data_for_blueprint(pawn_bp):
    data = subobject_library.get_data(handle)
    component = subobject_library.get_object_for_blueprint(data, pawn_bp)
    if component:
        component_paths.add(component.get_class().get_path_name())
required_component_paths = {footstep_class.get_path_name(), movement_class.get_path_name(), identifier_class.get_path_name()}
require(required_component_paths.issubset(component_paths),
        'BP_Thunderlord lacks one of Hyper Footsteps, Extended Movement, or Actor Identifier')

team_actors = [
    ('Showcase_Teammate_Blue', blue, red, 1),
    ('Showcase_Enemy_Red', red, blue, 3),
]
for label, own_tag, enemy_tag, stencil in team_actors:
    actor = next((item for item in actors if item.get_actor_label() == label), None)
    require(actor is not None, 'Missing ' + label)
    identifiers = actor.get_components_by_class(identifier_class)
    require(bool(identifiers), label + ' lacks Actor Identifier')
    identifier = identifiers[0]
    actor_id = identifier.get_editor_property('Actor ID')
    require(str(unreal.GameplayTagLibrary.get_tag_name(actor_id)) == 'NPC.ID', label + ' Actor ID is not NPC.ID')
    affiliations = identifier.get_editor_property('Own Affiliations')
    affiliation_names = {str(unreal.GameplayTagLibrary.get_tag_name(tag))
                         for tag in unreal.GameplayTagLibrary.break_gameplay_tag_container(affiliations)}
    require(str(unreal.GameplayTagLibrary.get_tag_name(own_tag)) in affiliation_names,
            label + ' has the wrong team')
    require(bool(actor.get_components_by_class(footstep_class)), label + ' lacks Hyper footsteps')
    require(bool(actor.get_components_by_class(movement_class)), label + ' lacks Hyper Extended Movement')
    meshes = actor.get_components_by_class(unreal.SkeletalMeshComponent)
    require(bool(meshes), label + ' has no skeletal mesh')
    for component in meshes:
        require(component.get_editor_property('render_custom_depth'), label + ' mesh is not outlined')
        require(component.get_editor_property('custom_depth_stencil_value') == stencil,
                label + ' has the wrong Hyper stencil value')

world_outliner = next((item for item in actors if item.get_actor_label() == 'Hyper_Outliner_PostProcess'), None)
require(world_outliner is not None and world_outliner.get_class().get_path_name().startswith(WORLD_OUTLINER_PATH),
        'Hyper outline post-process actor is missing')
post_process = world_outliner.get_components_by_class(unreal.PostProcessComponent)
require(bool(post_process), 'Hyper world outliner has no PostProcessComponent')
require(post_process[0].get_editor_property('unbound'), 'Hyper outline post-process is not unbound')

controller_bp = unreal.load_asset(CONTROLLER_PATH)
outliner_class = unreal.load_class(None, OUTLINER_PATH + '.AC_PC_Outliner_C')
require(outliner_class is not None, 'Hyper outliner component class is missing')
controller_components = set()
for handle in subsystem.k2_gather_subobject_data_for_blueprint(controller_bp):
    data = subobject_library.get_data(handle)
    component = subobject_library.get_object_for_blueprint(data, controller_bp)
    if component:
        controller_components.add(component.get_class().get_path_name())
require(outliner_class.get_path_name() in controller_components, 'Player controller lacks Hyper Outliner')

sequence_names = [
    'MX_Walk_Forward', 'MX_Walk_Right', 'MX_Walk_Backward', 'MX_Walk_Left',
    'MX_Run_Forward', 'MX_Run_Right', 'MX_Run_Backward', 'MX_Run_Left',
    'MX_Crouch_Forward', 'MX_Crouch_Right', 'MX_Crouch_Backward', 'MX_Crouch_Left',
]
for name in sequence_names:
    sequence = unreal.load_asset('/Game/Thunderlord/Zeus/Animations/Mixamo/' + name)
    require(sequence is not None, 'Missing animation ' + name)
    notifies = unreal.AnimationLibrary.get_animation_notify_events(sequence)
    require(len(notifies) >= 2, name + ' has fewer than two Hyper footstep notifies')

unreal.log_warning('HYPER_SHOWCASE_VALIDATED first_person=true mesh_offset=-96 blue_stencil=1 red_stencil=3 notify_sequences=12')
