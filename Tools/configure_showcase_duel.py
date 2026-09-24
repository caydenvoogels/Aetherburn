"""Replace the Showcase practice dummies with one AI-controlled duel opponent."""
import unreal

MAP = "/Game/Volcanic_temple/Levels/L_Showcase"
PLAYER_BLUEPRINT = "/Game/Thunderlord/Blueprints/BP_Thunderlord"
AI_CONTROLLER_PATH = "/Script/Aetherburn.AetherburnDuelAIController"
OLD_DUMMY_LABELS = {"Showcase_Teammate_Blue", "Showcase_Enemy_Red"}
ENEMY_LABEL = "Showcase_DuelEnemy"

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()

player_start = next((actor for actor in actors if isinstance(actor, unreal.PlayerStart)), None)
if player_start is None:
    raise RuntimeError("Showcase map has no PlayerStart")

existing_enemy = next((actor for actor in actors if actor.get_actor_label() == ENEMY_LABEL), None)
red_dummy = next((actor for actor in actors if actor.get_actor_label() == "Showcase_Enemy_Red"), None)
spawn_reference = existing_enemy or red_dummy
if spawn_reference is None:
    raise RuntimeError("Could not find the existing red dummy to use as the duel spawn point")

enemy_location = spawn_reference.get_actor_location()
player_location = player_start.get_actor_location()
enemy_rotation = unreal.MathLibrary.find_look_at_rotation(enemy_location, player_location)
player_rotation = unreal.MathLibrary.find_look_at_rotation(player_location, enemy_location)

controller_class = unreal.load_class(None, AI_CONTROLLER_PATH)
blueprint = unreal.load_asset(PLAYER_BLUEPRINT)
enemy_class = blueprint.generated_class() if blueprint else None
if controller_class is None or enemy_class is None:
    raise RuntimeError(f"Could not load duel classes: controller={controller_class}, pawn={enemy_class}")

auto_possess_enum = getattr(unreal, "AutoPossessAI", None)
auto_possess = getattr(auto_possess_enum, "PLACED_IN_WORLD_OR_SPAWNED", None) if auto_possess_enum else None
if auto_possess is None:
    raise RuntimeError(f"Could not resolve AutoPossessAI enum: {auto_possess_enum}")

for actor in actors:
    if actor.get_actor_label() in OLD_DUMMY_LABELS or actor.get_actor_label() == ENEMY_LABEL:
        actor_subsystem.destroy_actor(actor)

enemy = actor_subsystem.spawn_actor_from_class(enemy_class, enemy_location, enemy_rotation)
if enemy is None:
    raise RuntimeError("Failed to spawn the duel enemy")
enemy.set_actor_label(ENEMY_LABEL)
enemy.set_editor_property("ai_controller_class", controller_class)
enemy.set_editor_property("auto_possess_ai", auto_possess)
enemy.set_editor_property("is_damage_dummy", False)
for mesh in enemy.get_components_by_class(unreal.SkeletalMeshComponent):
    mesh.set_editor_property("render_custom_depth", True)
    mesh.set_editor_property("custom_depth_stencil_value", 3)
player_start.set_actor_rotation(player_rotation, True)

if enemy.get_editor_property("ai_controller_class") != controller_class:
    raise RuntimeError("Duel enemy did not retain its AI controller class")
if enemy.get_editor_property("auto_possess_ai") != auto_possess:
    raise RuntimeError("Duel enemy did not retain Auto Possess AI")
if enemy.get_editor_property("is_damage_dummy"):
    raise RuntimeError("Duel enemy retained damage-dummy behavior")
remaining_labels = {actor.get_actor_label() for actor in actor_subsystem.get_all_level_actors()}
if any(label in remaining_labels for label in OLD_DUMMY_LABELS):
    raise RuntimeError("A practice dummy was not removed")

if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP):
    raise RuntimeError(f"Failed to save duel setup to {MAP}")

unreal.log_warning(
    f"SHOWCASE_DUEL_READY enemy={ENEMY_LABEL} class={enemy.get_class().get_path_name()} "
    f"controller={controller_class.get_path_name()} location={enemy_location} "
    f"respawn_at_player_start=true removed_dummies={sorted(OLD_DUMMY_LABELS)}")
