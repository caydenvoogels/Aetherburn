"""Rebuild the Thunderlord Anim Blueprint with the upper-body throw slot."""
import unreal

lib = unreal.EditorAssetLibrary
load = lib.load_asset
paths = {
    "blueprint": "/Game/Thunderlord/Zeus/AnimationMap/ABP_Thunderlord",
    "standing": "/Game/Thunderlord/Zeus/AnimationMap/BS_Thunderlord_WalkSprint",
    "crouch": "/Game/Thunderlord/Zeus/AnimationMap/BS_Thunderlord_Crouch",
    "data": "/Game/Thunderlord/Zeus/AnimationMap/DA_ThunderlordAnimations",
}
assets = {key: load(path) for key, path in paths.items()}
if any(asset is None for asset in assets.values()):
    raise RuntimeError(f"Missing animation-map assets: {paths}")

actions = assets["data"].get_editor_property("actions")
clips = {
    action: load(f"/Game/Thunderlord/Zeus/Animations/Mixamo/{asset}")
    for action, asset in (("Slide", "MX_Slide_InPlace"), ("Jump", "MX_Jump_InPlace"),
                          ("Fall", "MX_Fall_InPlace"))
}
if any(clip is None for clip in clips.values()):
    raise RuntimeError("One or more in-place movement clips are missing")

if not unreal.ThunderlordContentLibrary.configure_animation_blueprint(
        assets["blueprint"], assets["standing"], assets["crouch"],
        clips["Slide"], clips["Jump"], clips["Fall"]):
    raise RuntimeError("Could not compile the layered locomotion and upper-body throw graph")

anim_cdo = unreal.get_default_object(assets["blueprint"].generated_class())
anim_cdo.set_editor_property("throw_release_fraction", 0.30)
anim_cdo.set_editor_property("throw_play_rate", 1.5)
lib.save_loaded_asset(assets["blueprint"])
unreal.log_warning(
    "THUNDERLORD_LAYERED_THROW_GRAPH_READY slot=UpperBodyThrow "
    f"release_fraction={anim_cdo.get_editor_property('throw_release_fraction')} "
    f"play_rate={anim_cdo.get_editor_property('throw_play_rate')}")
