from pathlib import Path
import runpy
import unreal

root = Path(__file__).resolve().parent
for name in ['import_slide_clips.py', 'retarget_movement_library.py']:
    unreal.log_warning('MOVEMENT_SETUP_STAGE ' + name)
    runpy.run_path(str(root / name), run_name='__main__')
