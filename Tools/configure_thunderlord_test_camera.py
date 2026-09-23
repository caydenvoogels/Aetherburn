"""Save stable third-person test-view defaults into BP_Thunderlord."""
import unreal

path = "/Game/Thunderlord/Blueprints/BP_Thunderlord"
library = unreal.EditorAssetLibrary
blueprint = library.load_asset(path)
if not blueprint:
    raise RuntimeError(f"Missing Thunderlord pawn Blueprint: {path}")

default_object = unreal.get_default_object(blueprint.generated_class())
boom = next((component for component in default_object.get_components_by_class(unreal.SpringArmComponent)
             if component.get_name() == "Camera Boom"), None)
body = next((component for component in default_object.get_components_by_class(unreal.SkeletalMeshComponent)
             if component.get_name() == "CharacterMesh0"), None)
if not boom or not body:
    raise RuntimeError("BP_Thunderlord is missing its camera boom or character mesh")

boom.set_editor_property("target_arm_length", 360.0)
boom.set_editor_property("socket_offset", unreal.Vector(0.0, 55.0, 0.0))
boom.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 58.0))
boom.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
boom.set_editor_property("do_collision_test", False)
boom.set_editor_property("use_pawn_control_rotation", True)
boom.set_editor_property("inherit_pitch", True)
boom.set_editor_property("inherit_yaw", True)
boom.set_editor_property("inherit_roll", False)
boom.set_editor_property("enable_camera_lag", False)
boom.set_editor_property("enable_camera_rotation_lag", False)
camera = next((component for component in default_object.get_components_by_class(unreal.CameraComponent)
               if component.get_name() == "First Person Camera"), None)
if not camera:
    raise RuntimeError("BP_Thunderlord is missing its view camera")
camera.set_editor_property("use_pawn_control_rotation", False)
body.set_editor_property("owner_no_see", False)
body.set_editor_property("only_owner_see", False)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
library.save_loaded_asset(blueprint)
unreal.log_warning(
    "THUNDERLORD_TEST_CAMERA_READY "
    f"arm={boom.get_editor_property('target_arm_length')} "
    f"collision={boom.get_editor_property('do_collision_test')} "
    f"control_rotation={boom.get_editor_property('use_pawn_control_rotation')} "
    f"owner_no_see={body.get_editor_property('owner_no_see')}")
