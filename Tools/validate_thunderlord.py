"""Validate that Thunderlord's animation map uses only Zeus Mixamo assets."""
import unreal

SKELETON_PATH = "/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig_Skeleton.SKM_ZeusMaxiamoRig_Skeleton"
MIXAMO_ROOT = "/Game/Thunderlord/Zeus/Animations/Mixamo"
MAP_PATH = "/Game/Thunderlord/Zeus/AnimationMap/DA_ThunderlordAnimations.DA_ThunderlordAnimations"

library = unreal.EditorAssetLibrary
skeleton = unreal.load_asset(SKELETON_PATH)
data = unreal.load_asset(MAP_PATH)
if not skeleton or not data:
    raise RuntimeError("Zeus skeleton or Mixamo animation map is missing")

asset_paths = library.list_assets(MIXAMO_ROOT, recursive=True, include_folder=False)
sequences = []
for path in asset_paths:
    animation = library.load_asset(path)
    if isinstance(animation, unreal.AnimSequence):
        if animation.get_editor_property("skeleton") != skeleton:
            raise RuntimeError(f"Mixamo animation uses the wrong skeleton: {path}")
        if animation.get_play_length() <= 0:
            raise RuntimeError(f"Mixamo animation has no keys: {path}")
        sequences.append(path)

required = {
    "MX_Idle", "MX_Walk_Forward", "MX_Walk_Backward", "MX_Walk_Left", "MX_Walk_Right",
    "MX_Run_Forward", "MX_Run_Backward", "MX_Run_Left", "MX_Run_Right",
    "MX_Crouch_Idle", "MX_Crouch_Forward", "MX_Crouch_Backward", "MX_Crouch_Left", "MX_Crouch_Right",
    "MX_Jump", "MX_Fall", "MX_Land", "MX_Slide", "MX_Slide_InPlace",
    "MX_Jump_InPlace", "MX_Fall_InPlace",
}
found = {path.rsplit("/", 1)[-1].split(".", 1)[0] for path in sequences}
missing = required - found
if missing:
    raise RuntimeError("Required Mixamo animations are missing: " + ", ".join(sorted(missing)))

standing = data.get_editor_property("walk_sprint")
crouching = data.get_editor_property("crouch")
if not standing or standing.get_editor_property("skeleton") != skeleton:
    raise RuntimeError("Standing blend space has the wrong skeleton")
if not crouching or crouching.get_editor_property("skeleton") != skeleton:
    raise RuntimeError("Crouch blend space has the wrong skeleton")

actions = data.get_editor_property("actions")
expected_actions = {"Slide", "Jump", "Fall"}
action_names = {str(name) for name in actions.keys()}
if action_names != expected_actions:
    raise RuntimeError(f"Unexpected action map keys: {sorted(action_names)}")
for name, clip in actions.items():
    if not clip or not clip.get_path_name().startswith(MIXAMO_ROOT + "/"):
        raise RuntimeError(f"Action {name} is not a Mixamo animation: {clip}")
for action_name, asset_name in (("Slide", "MX_Slide_InPlace"), ("Jump", "MX_Jump_InPlace"),
                                ("Fall", "MX_Fall_InPlace")):
    if actions[action_name].get_name() != asset_name:
        raise RuntimeError(f"{action_name} must use {asset_name}")
for name in ("MX_Slide_InPlace", "MX_Jump_InPlace", "MX_Fall_InPlace"):
    animation = library.load_asset(f"{MIXAMO_ROOT}/{name}.{name}")
    if not animation or not unreal.ThunderlordContentLibrary.is_animation_in_place(animation, True, 0.1):
        raise RuntimeError(f"Generated animation still translates its root away from the capsule: {name}")
anim_bp = library.load_asset("/Game/Thunderlord/Zeus/AnimationMap/ABP_Thunderlord.ABP_Thunderlord")
if not anim_bp:
    raise RuntimeError("Thunderlord Animation Blueprint was not generated")
generated = anim_bp.generated_class()
if not generated:
    raise RuntimeError("Thunderlord Anim Blueprint did not compile to a generated class")

unreal.log_warning(f"THUNDERLORD_MIXAMO_VALIDATED_SEQUENCES={len(sequences)}")
unreal.log_warning("THUNDERLORD_MIXAMO_MAP_OK")
