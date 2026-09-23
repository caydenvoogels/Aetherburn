import unreal

MAP = '/Game/Volcanic_temple/Levels/L_Showcase'
world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
if starts:
    start = starts[0]
    start_location = start.get_actor_location()
    nearby = []
    for actor in actors:
        if actor == start or not isinstance(actor, unreal.StaticMeshActor):
            continue
        origin, extent = actor.get_actor_bounds(True)
        dx = max(abs(origin.x - start_location.x) - extent.x, 0.0)
        dy = max(abs(origin.y - start_location.y) - extent.y, 0.0)
        dz = max(abs(origin.z - start_location.z) - extent.z - 100.0, 0.0)
        distance = (dx * dx + dy * dy + dz * dz) ** 0.5
        if distance < 300.0:
            nearby.append((distance, actor.get_actor_label(), origin, extent))
    for distance, name, origin, extent in sorted(nearby)[:30]:
        unreal.log_warning(
            f'SPAWN_CLEARANCE actor={name} surface_distance={distance:.1f} '
            f'origin={origin} extent={extent}')

for actor in actors:
    name = actor.get_actor_label()
    class_name = actor.get_class().get_name()
    if ('PlayerStart' in class_name or 'Start' in name or 'Spawn' in name or
            'Floor' in name or 'Ground' in name or 'Showcase' in name):
        origin, extent = actor.get_actor_bounds(False)
        unreal.log_warning(
            f'SHOWCASE_ACTOR name={name} class={class_name} '
            f'location={actor.get_actor_location()} rotation={actor.get_actor_rotation()} '
            f'origin={origin} extent={extent}')

unreal.log_warning(f'SHOWCASE_ACTOR_COUNT={len(actors)}')
