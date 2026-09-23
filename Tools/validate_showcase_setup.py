import unreal

MAP = '/Game/Volcanic_temple/Levels/L_Showcase'
THUNDERLORD_MESH = '/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig'

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
settings = world.get_world_settings()
game_mode = settings.get_editor_property('default_game_mode')
starts = [actor for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
          if isinstance(actor, unreal.PlayerStart)]

if not game_mode or game_mode.get_name() != 'ThunderlordGameMode':
    raise RuntimeError(f'Unexpected Showcase game mode: {game_mode}')
if len(starts) != 1:
    raise RuntimeError(f'Expected one Showcase PlayerStart, found {len(starts)}')

game_mode_cdo = unreal.get_default_object(game_mode)
pawn = game_mode_cdo.get_editor_property('default_pawn_class')
if not pawn or pawn.get_name() != 'BP_Thunderlord_C':
    raise RuntimeError(f'Unexpected default pawn: {pawn}')

pawn_cdo = unreal.get_default_object(pawn)
mesh_components = pawn_cdo.get_components_by_class(unreal.SkeletalMeshComponent)
body_mesh = next((component for component in mesh_components
                  if component.get_name() == 'CharacterMesh0'), None)
if not body_mesh:
    raise RuntimeError('BP_Thunderlord has no CharacterMesh0 component')
mesh_asset = body_mesh.get_editor_property('skeletal_mesh_asset')
if not mesh_asset or mesh_asset.get_path_name() != THUNDERLORD_MESH:
    raise RuntimeError(f'BP_Thunderlord uses unexpected body mesh: {mesh_asset}')
relative_location = body_mesh.get_editor_property('relative_location')
if abs(relative_location.z + 60.0) > 0.01:
	raise RuntimeError(f'BP_Thunderlord should apply a -60-unit mesh offset: {relative_location}')
anim_class = body_mesh.get_editor_property('anim_class')
if not anim_class or anim_class.get_name() != 'ABP_Thunderlord_C':
    raise RuntimeError(f'BP_Thunderlord is not using the state-machine Anim Blueprint: {anim_class}')

unreal.log_warning(
    f'SHOWCASE_VALID game_mode={game_mode.get_name()} pawn={pawn.get_name()} '
    f'mesh={mesh_asset.get_path_name()} spawn={starts[0].get_actor_location()}')
