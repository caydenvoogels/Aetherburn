import unreal

SOURCE_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"
TARGET_MESH_PATH = "/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig"
SOURCE_ANIM_BP_PATH = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed"
OUTPUT_PATH = "/Game/Thunderlord/Zeus/Animations"


def load_required(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required asset not found: {path}")
    return asset


def create_or_load(asset_path, asset_class, factory_class):
    existing = unreal.load_asset(asset_path)
    if existing:
        return existing
    package_path, asset_name = asset_path.rsplit("/", 1)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory_class()
    )


source_mesh = load_required(SOURCE_MESH_PATH)
target_mesh = load_required(TARGET_MESH_PATH)

source_rig = create_or_load(
    "/Game/Thunderlord/Rigs/IK_Manny",
    unreal.IKRigDefinition,
    unreal.IKRigDefinitionFactory,
)
target_rig = create_or_load(
    "/Game/Thunderlord/Zeus/Rigs/IK_Zeus",
    unreal.IKRigDefinition,
    unreal.IKRigDefinitionFactory,
)

source_controller = unreal.IKRigController.get_controller(source_rig)
source_controller.set_skeletal_mesh(source_mesh)
source_controller.apply_auto_generated_retarget_definition()
source_controller.set_retarget_root("pelvis")

target_controller = unreal.IKRigController.get_controller(target_rig)
target_controller.set_skeletal_mesh(target_mesh)
target_controller.apply_auto_generated_retarget_definition()
target_controller.set_retarget_root("Hips")

retargeter = create_or_load(
    "/Game/Thunderlord/Zeus/Rigs/RTG_Manny_Zeus",
    unreal.IKRetargeter,
    unreal.IKRetargetFactory,
)
retarget_controller = unreal.IKRetargeterController.get_controller(retargeter)
retarget_controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
retarget_controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
retarget_controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
retarget_controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
retarget_controller.remove_all_ops()
retarget_controller.add_default_ops()
retarget_controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
# Keep the Zeus FBX reference pose intact. Auto-aligning all target bones had
# been shifting the hips and producing collapsed locomotion previews.

source_anim_bp = load_required(SOURCE_ANIM_BP_PATH)
inputs = unreal.IKRetargetBatchOperationInputs()
inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(SOURCE_ANIM_BP_PATH)]
inputs.source_mesh = source_mesh
inputs.target_mesh = target_mesh
inputs.ik_retarget_asset = retargeter
inputs.prefix = "TL_"
inputs.target_path = OUTPUT_PATH
inputs.include_referenced_assets = True
inputs.overwrite_existing_files = True
created = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
if not created:
    raise RuntimeError("Retargeting produced no assets")

for asset in (source_rig, target_rig, retargeter):
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.EditorAssetLibrary.save_directory("/Game/Thunderlord/Zeus", only_if_is_dirty=False, recursive=True)

unreal.log_warning(f"THUNDERLORD_RETARGET_CREATED={len(created)}")
for asset_data in created:
    unreal.log_warning(f"THUNDERLORD_ASSET={asset_data.package_name}")
