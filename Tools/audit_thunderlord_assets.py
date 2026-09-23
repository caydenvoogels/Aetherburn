"""Check target skeletons, runtime defaults, and old Meshy asset references."""
import unreal

lib = unreal.EditorAssetLibrary
mesh = unreal.load_asset('/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig.SKM_ZeusMaxiamoRig')
skeleton = unreal.load_asset('/Game/Thunderlord/ZeusImport/SKM_ZeusMaxiamoRig_Skeleton.SKM_ZeusMaxiamoRig_Skeleton')
animation_root = '/Game/Thunderlord/Zeus/Animations'
movement_root = '/Game/Thunderlord/Zeus/Movement'
map_root = '/Game/Thunderlord/Zeus/AnimationMap'
if not mesh or not skeleton:
    raise RuntimeError('Zeus skeletal mesh or skeleton is missing')

for root in (animation_root, movement_root):
    for path in lib.list_assets(root, recursive=True, include_folder=False):
        asset = lib.load_asset(path)
        if isinstance(asset, unreal.AnimSequence):
            if asset.get_editor_property('skeleton') != skeleton:
                raise RuntimeError(f'Animation targets a different skeleton: {path}')
            unreal.log_warning(f'ZEUS_ANIM_OK {path} duration={asset.get_play_length():.3f}')

data = unreal.load_asset(map_root + '/DA_ThunderlordAnimations.DA_ThunderlordAnimations')
if not data:
    raise RuntimeError('Zeus animation data asset is missing')
if data.get_editor_property('walk_sprint').get_editor_property('skeleton') != skeleton:
    raise RuntimeError('Standing blend space has wrong skeleton')
if data.get_editor_property('crouch').get_editor_property('skeleton') != skeleton:
    raise RuntimeError('Crouch blend space has wrong skeleton')
for name, clip in data.get_editor_property('actions').items():
    if clip.get_editor_property('skeleton') != skeleton:
        raise RuntimeError(f'Action {name} has wrong skeleton: {clip.get_path_name()}')
unreal.log_warning('ZEUS_ANIMATION_SET_OK')

bp = unreal.load_asset('/Game/Thunderlord/Blueprints/BP_Thunderlord.BP_Thunderlord')
pawn = unreal.get_default_object(bp.generated_class())
body = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent)
            if c.get_name() == 'CharacterMesh0')
if body.get_editor_property('skeletal_mesh_asset') != mesh:
    raise RuntimeError('BP_Thunderlord does not use Zeus mesh')
unreal.log_warning(f'ZEUS_PLAYER_DEFAULT_OK mesh={mesh.get_path_name()} anim={body.get_editor_property("anim_class")}')

referencers = getattr(lib, 'find_package_referencers_for_asset', None)
unreal.log_warning(f'REFERENCER_API_AVAILABLE={bool(referencers)}')
if not referencers:
    raise RuntimeError('Cannot safely check package referencers using Unreal Python')
legacy_roots = [
    '/Game/MeshyImports',
    '/Game/Thunderlord/Animations',
    '/Game/Thunderlord/Movement',
    '/Game/Thunderlord/AnimationMap',
    '/Game/Thunderlord/ZeusSource',
    '/Game/Thunderlord/ZeusSourceLegacy',
    '/Game/Thunderlord/ZeusSourceFbxLegacy',
]
for root in legacy_roots:
    for path in lib.list_assets(root, recursive=True, include_folder=False):
        refs = referencers(path, True)
        if refs:
            unreal.log_warning(f'LEGACY_ASSET_REFERENCED {path} refs={refs}')
        else:
            unreal.log_warning(f'LEGACY_ASSET_CLEAR {path}')
unreal.log_warning('THUNDERLORD_ASSET_AUDIT_DONE')
for path in lib.list_assets('/Game/Thunderlord/Zeus', recursive=False, include_folder=False):
    unreal.log_warning(f'ZEUS_ROOT_ASSET {path} refs={referencers(path, True)}')
