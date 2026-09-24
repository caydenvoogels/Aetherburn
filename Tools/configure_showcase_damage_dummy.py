"""Mark the red Showcase Zeus as a stationary, damage-logging target."""
import unreal

MAP = "/Game/Volcanic_temple/Levels/L_Showcase"
TARGET_LABEL = "Showcase_Enemy_Red"

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
target = next((actor for actor in actors if actor.get_actor_label() == TARGET_LABEL), None)
if target is None:
    raise RuntimeError(f"Could not find Showcase target: {TARGET_LABEL}")

property_name = "is_damage_dummy"
if not isinstance(target, unreal.AetherburnCharacter):
    raise RuntimeError(
        f"{TARGET_LABEL} must be an AetherburnCharacter, got {target.get_class().get_path_name()}"
    )

target.set_editor_property(property_name, True)
if not target.get_editor_property(property_name):
    raise RuntimeError(f"Failed to enable damage dummy behavior on {TARGET_LABEL}")

saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
if not saved:
    raise RuntimeError(f"Failed to save Showcase map: {MAP}")
unreal.log_warning(
    f"SHOWCASE_DAMAGE_DUMMY_READY name={TARGET_LABEL} class={target.get_class().get_path_name()}"
)
