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

footstep_class = unreal.load_class(None, FOOTSTEP_PATH + '.AC_CH_Footsteps_C')
movement_class = unreal.load_class(None, MOVEMENT_PATH + '.AC_CH_Extended_Movement_Base_C')
identifier_class = unreal.load_class(None, IDENTIFIER_PATH + '.AC_Actor_Identifier_C')
require(footstep_class and movement_class and identifier_class, 'A required Hyper component class is missing')

pawn_bp = unreal.load_asset(PAWN_PATH)
pawn_cdo = unreal.get_default_object(pawn_bp.generated_class())
camera_booms = pawn_cdo.get_components_by_class(unreal.SpringArmComponent)
require(bool(camera_booms), 'BP_Thunderlord has no camera boom')
camera_boom = camera_booms[0]
require(abs(camera_boom.get_editor_property('target_arm_length') - 360.0) < 0.01,
        'Showcase camera is not configured for the third-person test view')
camera_offset = camera_boom.get_editor_property('socket_offset')
require(abs(camera_offset.x) < 0.01 and abs(camera_offset.y - 55.0) < 0.01 and abs(camera_offset.z) < 0.01,
        'Showcase camera does not have the expected shoulder offset')
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

enemy = next((item for item in actors if item.get_actor_label() == 'Showcase_DuelEnemy'), None)
require(enemy is not None, 'Missing Showcase_DuelEnemy')
enemy_controller = enemy.get_editor_property('ai_controller_class')
require(enemy_controller and enemy_controller.get_path_name() == '/Script/Aetherburn.AetherburnDuelAIController',
        'Showcase duel enemy has the wrong AI controller')
require(enemy.get_editor_property('auto_possess_ai') == unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED,
        'Showcase duel enemy does not auto-possess AI')
require(not enemy.get_editor_property('is_damage_dummy'), 'Showcase duel enemy is still a damage dummy')
require(not any(item.get_actor_label() in {'Showcase_Teammate_Blue', 'Showcase_Enemy_Red'} for item in actors),
        'Old practice dummies remain in the Showcase')
require(bool(enemy.get_components_by_class(footstep_class)), 'Duel enemy lacks Hyper footsteps')
require(bool(enemy.get_components_by_class(movement_class)), 'Duel enemy lacks Hyper Extended Movement')
enemy_meshes = enemy.get_components_by_class(unreal.SkeletalMeshComponent)
require(bool(enemy_meshes), 'Duel enemy has no skeletal mesh')
for component in enemy_meshes:
    require(component.get_editor_property('render_custom_depth'), 'Duel enemy mesh is not outlined')
    require(component.get_editor_property('custom_depth_stencil_value') == 3,
            'Duel enemy does not use the red outline stencil')

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

unreal.log_warning('HYPER_SHOWCASE_VALIDATED third_person=true duel_enemy=true blue_stencil=1 red_stencil=3 notify_sequences=12')
