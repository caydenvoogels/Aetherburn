import unreal

MAP = '/Game/Volcanic_temple/Levels/L_Showcase'
GAME_MODE = '/Script/Aetherburn.ThunderlordGameMode'

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
world.get_world_settings().set_editor_property('default_game_mode', unreal.ThunderlordGameMode)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
starts = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.PlayerStart)]
if starts:
    start = starts[0]
else:
    # Central open ground in the showcase layout, lifted above the surface so
    # the capsule settles cleanly when play begins.
    start = actors.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(5000.0, -1000.0, 220.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    start.set_actor_label('PlayerStart_Aetherburn')

saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
unreal.log_warning(
    f'SHOWCASE_CONFIGURED saved={saved} game_mode={GAME_MODE} '
    f'spawn={start.get_actor_location()}')
