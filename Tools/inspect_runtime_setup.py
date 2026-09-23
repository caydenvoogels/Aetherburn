import unreal

def prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as error:
        return f'<unavailable: {error}>'

mesh = unreal.load_asset('/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig')
unreal.log_warning(f'MESH_METHODS={[name for name in dir(mesh) if "bound" in name.lower() or "extent" in name.lower()]}' )
unreal.log_warning(f'MESH_IMPORTED_BOUNDS={prop(mesh, "imported_bounds")}')
unreal.log_warning(
    f'MESH_ASSET_DIAGNOSTICS methods={[name for name in dir(mesh) if "bounds" in name.lower() or "lod" in name.lower()]} '
    f'skeleton={prop(mesh, "skeleton")} bounds={mesh.get_bounds()}')

bp = unreal.load_asset('/Game/Thunderlord/Blueprints/BP_Thunderlord.BP_Thunderlord')
cdo = unreal.get_default_object(bp.generated_class())
for component in cdo.get_components_by_class(unreal.ActorComponent):
    if isinstance(component, (unreal.SkeletalMeshComponent, unreal.CameraComponent, unreal.SpringArmComponent)):
        unreal.log_warning(
            f'PLAYER_COMPONENT name={component.get_name()} class={component.get_class().get_name()} '
            f'visible={prop(component, "visible")} hidden={prop(component, "hidden_in_game")} '
            f'location={prop(component, "relative_location")} rotation={prop(component, "relative_rotation")} '
            f'mesh={prop(component, "skeletal_mesh_asset")} owner_no_see={prop(component, "owner_no_see")} '
            f'only_owner_see={prop(component, "only_owner_see")} cast_shadow={prop(component, "cast_shadow")} '
            f'first_person_type={prop(component, "first_person_primitive_type")} '
            f'enable_first_person_fov={prop(component, "enable_first_person_field_of_view")} '
            f'anim_class={prop(component, "anim_class")} arm_length={prop(component, "target_arm_length")} '
            f'arm_collision={prop(component, "do_collision_test")} '
            f'scale={prop(component, "relative_scale3d")} '
            f'bounds={prop(component, "bounds")} '
            f'animation_set={prop(prop(component, "anim_class"), "animation_set")} '
        )

for map_path in ['/Game/FirstPerson/Lvl_FirstPerson', '/Game/Volcanic_temple/Levels/L_Showcase']:
    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    settings = world.get_world_settings()
    game_mode = prop(settings, 'default_game_mode')
    unreal.log_warning(f'MAP_SETUP map={map_path} game_mode={game_mode}')
    if game_mode:
        game_mode_cdo = unreal.get_default_object(game_mode)
        unreal.log_warning(f'MAP_PAWN map={map_path} pawn={prop(game_mode_cdo, "default_pawn_class")}')
