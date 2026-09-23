"""Inventory animation clip families and skeletons before rebuilding the map."""
from pathlib import Path
import unreal

lib = unreal.EditorAssetLibrary
roots = [
    '/Game/Characters/Mannequins',
    '/Game/ThirdParty',
    '/Game/Thunderlord/Zeus',
]
records = []
for root in roots:
    for path in lib.list_assets(root, recursive=True, include_folder=False):
        asset = lib.load_asset(path)
        if isinstance(asset, unreal.AnimSequence):
            skeleton = asset.get_editor_property('skeleton')
            skel_path = skeleton.get_path_name() if skeleton else '<none>'
            records.append((root, path, skel_path, f'{asset.get_play_length():.3f}'))

out = Path(unreal.Paths.project_saved_dir()) / 'Logs' / 'animation_sources.tsv'
out.write_text('root\tasset\tskeleton\tduration\n' + ''.join('\t'.join(row) + '\n' for row in records), encoding='utf-8')
unreal.log_warning(f'ANIMATION_SOURCE_INVENTORY count={len(records)} file={out}')
for root in roots:
    grouped = {}
    for source, path, skeleton, duration in records:
        if source == root:
            grouped[skeleton] = grouped.get(skeleton, 0) + 1
    unreal.log_warning(f'ANIMATION_SOURCE_GROUP root={root} groups={grouped}')

for mesh_path in [
    '/Game/ThirdParty/KayKit/Source/Mannequin_Medium.Mannequin_Medium',
    '/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig',
]:
    mesh = lib.load_asset(mesh_path)
    if mesh:
        unreal.log_warning(
            f'RIG_ROOTS mesh={mesh_path} hips_children={mesh.get_bone_children("Hips")} '
            f'root_children={mesh.get_bone_children("Root")}')

for fbx_path in [Path.home() / 'Downloads' / 'Untitled.fbx', Path.home() / 'Downloads' / 'ZeusMaxiamoRig.fbx']:
    if fbx_path.is_file():
        blob = fbx_path.read_bytes()
        tags = [b'AnimationStack', b'AnimationLayer', b'AnimationCurveNode', b'AnimationCurve', b'KeyTime', b'mixamorig']
        unreal.log_warning(f'FBX_TAGS {fbx_path.name} bytes={len(blob)} { {tag.decode(): blob.count(tag) for tag in tags} }')
