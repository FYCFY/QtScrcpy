import dmgbuild
import json
import os
import subprocess
import sys
import tempfile

current_file_path = os.path.dirname(os.path.realpath(__file__))
dmg_background_img = '%s/dmg-background.jpg' % current_file_path
app_path = '%s/../../build/QtScrcpy.app' % current_file_path
dmg_path = '%s/../../build/QtScrcpy.dmg' % current_file_path
app_name = 'QtScrcpy'
fetch_fastboot_script = os.path.abspath(os.path.join(current_file_path, '../../scripts/fetch_fastboot.py'))

def console_print(msg):
    print(msg)
    sys.stdout.flush()

def build_dmg():
    console_print('generate dmg settings')
    info = {
        'title': app_name,
        'icon-size': 120,
        'format': 'UDZO',
        'compression-level': 9,
        'window': {
            'position': {'x': 400, 'y': 200},
            'size': {'width': 780, 'height': 480},
        },
        'contents': [
            {
                'x': 223,
                'y': 227,
                'type': 'file',
                'path': app_path,
            },
            {
                'x': 550,
                'y': 227,
                'type': 'link',
                'path': '/Applications',
            },
        ],
    }

    if os.path.exists(dmg_background_img):
        info['background'] = dmg_background_img

    with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False) as file:
        json.dump(info, file)
        tmp_settings_path = file.name

    try:
        console_print('build dmg: %s' % dmg_path)
        dmgbuild.build_dmg(dmg_path, app_name, tmp_settings_path)
    finally:
        if os.path.exists(tmp_settings_path):
            os.remove(tmp_settings_path)

if __name__ == '__main__':
    console_print('prepare dmg package')
    console_print('fetch fastboot binary')
    dest_dir = os.path.join(app_path, 'Contents', 'MacOS')
    ret = subprocess.call([sys.executable, fetch_fastboot_script, '--platform', 'mac', '--dest', dest_dir])
    if ret != 0:
        console_print('failed to fetch fastboot binary')
        sys.exit(ret)
    build_dmg()
    if not os.path.exists(dmg_path):
        console_print('fail to create %s' % dmg_path)
        sys.exit(1)
    
    sys.exit(0)
