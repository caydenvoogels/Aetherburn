"""Check that the Showcase map contains one configured duel opponent."""
import unreal

MAP = "/Game/Volcanic_temple/Levels/L_Showcase"
AI_CONTROLLER_PATH = "/Script/Aetherburn.AetherburnDuelAIController"

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
enemies = [actor for actor in actors if actor.get_actor_label() == "Showcase_DuelEnemy"]
stale_dummies = [actor.get_actor_label() for actor in actors if actor.get_actor_label() in {
    "Showcase_Teammate_Blue", "Showcase_Enemy_Red"}]

if len(starts) != 1:
    raise RuntimeError(f"Expected one PlayerStart, found {len(starts)}")
if len(enemies) != 1:
    raise RuntimeError(f"Expected one Showcase_DuelEnemy, found {len(enemies)}")
if stale_dummies:
    raise RuntimeError(f"Old practice dummies remain in the map: {stale_dummies}")

enemy = enemies[0]
controller_class = enemy.get_editor_property("ai_controller_class")
if not controller_class or controller_class.get_path_name() != AI_CONTROLLER_PATH:
    raise RuntimeError(f"Unexpected duel AI controller: {controller_class}")
if enemy.get_editor_property("auto_possess_ai") != unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED:
    raise RuntimeError("Duel enemy will not auto-possess its AI controller")
if enemy.get_editor_property("is_damage_dummy"):
    raise RuntimeError("Duel enemy still has damage-dummy behavior enabled")

meshes = enemy.get_components_by_class(unreal.SkeletalMeshComponent)
if not meshes:
    raise RuntimeError("Duel enemy has no character mesh")
for mesh in meshes:
    if not mesh.get_editor_property("render_custom_depth") or mesh.get_editor_property("custom_depth_stencil_value") != 3:
        raise RuntimeError("Duel enemy is missing the red team outline")

unreal.log_warning(
    f"SHOWCASE_DUEL_VALID enemy={enemy.get_actor_label()} controller={controller_class.get_path_name()} "
    f"player_start={starts[0].get_actor_location()} enemy_location={enemy.get_actor_location()}")
