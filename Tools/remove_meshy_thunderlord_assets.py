"""Remove the superseded Meshy Thunderlord content after referencer checks."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
referencers = lib.find_package_referencers_for_asset
roots = [
    '/Game/MeshyImports',
    '/Game/Thunderlord/Animations',
    '/Game/Thunderlord/Movement',
    '/Game/Thunderlord/AnimationMap',
    '/Game/Thunderlord/ZeusSource',
    '/Game/Thunderlord/ZeusSourceLegacy',
    '/Game/Thunderlord/ZeusSourceFbxLegacy',
]
packages = {}
for root in roots:
    for path in lib.list_assets(root, recursive=True, include_folder=False):
        packages[path.rsplit('.', 1)[0]] = path
for path in [
    '/Game/Thunderlord/Zeus/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig',
    '/Game/Thunderlord/Zeus/SKM_ZeusMaxiamoRig_Skeleton.SKM_ZeusMaxiamoRig_Skeleton',
    '/Game/Thunderlord/Zeus/BakedMaterial.BakedMaterial',
]:
    if lib.does_asset_exist(path):
        packages[path.rsplit('.', 1)[0]] = path
for path in lib.list_assets('/Game/Thunderlord/Rigs', recursive=False, include_folder=False):
    name = path.rsplit('/', 1)[-1].split('.', 1)[0]
    if name == 'IK_Thunderlord' or name in {
        'RTG_Manny_Thunderlord', 'RTG_UAL1_Thunderlord', 'RTG_UAL2_Thunderlord'
    }:
        packages[path.rsplit('.', 1)[0]] = path

package_set = set(packages)
external = []
for package, asset_path in packages.items():
    refs = referencers(asset_path, True)
    outside = [ref for ref in refs if ref not in package_set]
    if outside:
        external.append((asset_path, outside))
if external:
    for asset_path, refs in external:
        unreal.log_error(f'MESHY_CLEANUP_BLOCKED {asset_path} external_referencers={refs}')
    raise RuntimeError('Old Meshy content still has live references outside the removal set')

# Remove the old target rig and retargeters first; then its map and animations;
# then the imported Meshy packages that those assets depended on.
explicit_rigs = [
    '/Game/Thunderlord/Rigs/RTG_Manny_Thunderlord',
    '/Game/Thunderlord/Rigs/RTG_UAL1_Thunderlord',
    '/Game/Thunderlord/Rigs/RTG_UAL2_Thunderlord',
    '/Game/Thunderlord/Rigs/IK_Thunderlord',
]
for path in explicit_rigs:
    if lib.does_asset_exist(path) and not lib.delete_asset(path):
        raise RuntimeError(f'Could not delete obsolete target rig asset: {path}')
for path in [
    '/Game/Thunderlord/Zeus/SKM_ZeusMaxiamoRig',
    '/Game/Thunderlord/Zeus/SKM_ZeusMaxiamoRig_Skeleton',
    '/Game/Thunderlord/Zeus/BakedMaterial',
]:
    if lib.does_asset_exist(path) and not lib.delete_asset(path):
        raise RuntimeError(f'Could not delete duplicate import asset: {path}')

for root in [
    '/Game/Thunderlord/AnimationMap',
    '/Game/Thunderlord/Animations',
    '/Game/Thunderlord/Movement',
    '/Game/Thunderlord/ZeusSource',
    '/Game/Thunderlord/ZeusSourceLegacy',
    '/Game/Thunderlord/ZeusSourceFbxLegacy',
]:
    if lib.does_directory_exist(root) and not lib.delete_directory(root):
        raise RuntimeError(f'Could not delete obsolete content folder: {root}')

for root in roots:
    if lib.does_directory_exist(root) and lib.list_assets(root, recursive=True, include_folder=False):
        raise RuntimeError(f'Assets remain after deleting {root}')
if lib.list_assets('/Game/MeshyImports', recursive=True, include_folder=False):
    raise RuntimeError('Unreal assets remain under /Game/MeshyImports')

# Unreal deletes imported packages, but leaves original FBX/FBM source files.
# Remove only this one superseded Meshy source and its sidecar textures.
content_root = Path(unreal.Paths.project_content_dir()).resolve()
meshy_root = (content_root / 'MeshyImports').resolve()
raw_files = [
    'Thunderlord of Olympus_Import0001.fbx',
    'Thunderlord of Olympus_Import0001.fbm/Meshy_AI_Thunderlord_of_Olympu_0923073003_texture.png',
    'Thunderlord of Olympus_Import0001.fbm/normal.png',
    'Thunderlord of Olympus_Import0001.fbm/texture_0_metallic.png',
    'Thunderlord of Olympus_Import0001.fbm/texture_0_normal.png',
    'Thunderlord of Olympus_Import0001.fbm/texture_0_roughness.png',
    'Thunderlord of Olympus_Import0001.fbm/texture_0.png',
]
for relative in raw_files:
    path = (meshy_root / relative).resolve()
    if meshy_root not in path.parents:
        raise RuntimeError(f'Unexpected source path: {path}')
    if path.exists():
        if path.suffix.lower() in {'.uasset', '.umap'}:
            raise RuntimeError(f'Refusing to remove Unreal package outside the Editor: {path}')
        path.unlink()
for relative in [
    'Thunderlord of Olympus_Import0001.fbm',
    'Thunderlord_of_Olympus_Import0001_fbm',
    'Import_20260922_085822',
    '.',
]:
    folder = meshy_root if relative == '.' else meshy_root / relative
    if folder.exists():
        if any(folder.iterdir()):
            raise RuntimeError(f'Unrecognized files remain in old Meshy folder: {folder}')
        folder.rmdir()
unreal.log_warning(f'MESHY_THUNDERLORD_REMOVED packages={len(packages)}')
