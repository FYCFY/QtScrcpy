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

def generate_dmg_info():
    info = {
        'title': app_name,
        'icon-size': 120,
        'background': dmg_background_img,
        'format': 'UDZO',
        'compression-level': 9,
        'window': {
            'position': {'x': 400, 'y': 200},
            'size': {'width': 780, 'height': 480}
        },
        'contents': [
            {
                'x': 223,
                'y': 227,
                'type': 'file',
                'path': app_path
            },
            {
                'x': 550,
                'y': 227,
                'type': 'link',
                'path': '/Applications'
            }
        ]
    }

    tmp = tempfile.NamedTemporaryFile('w', suffix='.json', delete=False)
    try:
        json.dump(info, tmp)
        tmp.flush()
        return tmp.name
    finally:
        tmp.close()

if __name__ == '__main__':
    console_print('fetch fastboot binary')
    dest_dir = os.path.join(app_path, 'Contents', 'MacOS')
    ret = subprocess.call([sys.executable, fetch_fastboot_script, '--platform', 'mac', '--dest', dest_dir])
    if ret != 0:
        console_print('failed to fetch fastboot binary')
        sys.exit(ret)

    console_print('generate dmg info')
    dmg_settings_path = generate_dmg_info()
    try:
        console_print('build dmg: %s' % dmg_path)
        dmgbuild.build_dmg(dmg_path, app_name, dmg_settings_path)
    finally:
        try:
            os.remove(dmg_settings_path)
        except OSError:
            pass

    if not os.path.exists(dmg_path):
        console_print('fail to create %s' % dmg_path)
        sys.exit(1)

    sys.exit(0)
