#!/usr/bin/env bash
set -euo pipefail

# 获取绝对路径，保证其他目录执行此脚本依然正确
{
    cd "$(dirname "$0")"
    script_path=$(pwd)
    cd -
} &>/dev/null

old_cd=$(pwd)
cd "$(dirname "$0")"

app_name="QtScrcpy"
app_path="$script_path/../../build/${app_name}.app"
dmg_path="$script_path/../../build/${app_name}.dmg"
fastboot_dest="$app_path/Contents/MacOS"
fetch_fastboot_py="$script_path/../scripts/fetch_fastboot.py"
python_bin=${PYTHON_BIN:-python3}

if ! command -v "$python_bin" >/dev/null 2>&1; then
    if command -v python >/dev/null 2>&1; then
        python_bin=python
    else
        echo "error: python interpreter not found" >&2
        exit 1
    fi
fi

if ! command -v hdiutil >/dev/null 2>&1; then
    echo "error: hdiutil command is required to create DMG packages" >&2
    exit 1
fi

if [ ! -d "$app_path" ]; then
    echo "error: $app_path does not exist" >&2
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo fetch fastboot binary
echo ---------------------------------------------------------------
"$python_bin" "$fetch_fastboot_py" --platform mac --dest "$fastboot_dest"

if [ -f "$dmg_path" ]; then
    rm -f "$dmg_path"
fi

tmp_dir=$(mktemp -d)
cleanup() {
    rm -rf "$tmp_dir"
}
trap cleanup EXIT

cp -R "$app_path" "$tmp_dir/"
ln -s /Applications "$tmp_dir/Applications"

pushd "$tmp_dir" >/dev/null
hdiutil create -volname "$app_name" -srcfolder "$tmp_dir" -format UDZO -quiet -ov "$dmg_path"
popd >/dev/null

echo
echo
echo ---------------------------------------------------------------
echo package created at $dmg_path
echo ---------------------------------------------------------------

cd "$old_cd"
exit 0
