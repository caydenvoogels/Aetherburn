"""Run focused Editor-side checks for the Zeus character replacement."""
from pathlib import Path
import runpy
import unreal

root = Path(__file__).resolve().parent
for script in [
    'audit_thunderlord_assets.py',
    'validate_thunderlord.py',
    'validate_showcase_setup.py',
]:
    unreal.log_warning(f'FINAL_VALIDATION_STAGE {script}')
    runpy.run_path(str(root / script), run_name='__main__')
unreal.log_warning('FINAL_THUNDERLORD_VALIDATION_OK')
