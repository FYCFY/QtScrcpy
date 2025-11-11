#!/bin/bash
# 获取绝对路径，保证其他目录执行此脚本依然正确

{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output

set -euo pipefail

# 设置当前目录，cd的目录影响接下来执行程序的工作目录
old_cd=$(pwd)
cd $(dirname "$0")

app_path="$script_path/../../build/QtScrcpy.app"
dmg_path="$script_path/../../build/QtScrcpy.dmg"

if [ ! -d "$app_path" ]; then
    echo "error: QtScrcpy.app not found at $app_path"
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo fetch fastboot binary
echo ---------------------------------------------------------------

python_cmd="python3"
if ! command -v "$python_cmd" >/dev/null 2>&1; then
    python_cmd="python"
fi

if ! command -v "$python_cmd" >/dev/null 2>&1; then
    echo "error: python interpreter not found"
    exit 1
fi

fastboot_dest="$app_path/Contents/MacOS"
"$python_cmd" "$script_path/../scripts/fetch_fastboot.py" --platform mac --dest "$fastboot_dest"

echo
echo
echo ---------------------------------------------------------------
echo create dmg package
echo ---------------------------------------------------------------

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/qtscrcpy_pkg.XXXXXX")
cleanup() {
    rm -rf "$tmp_dir"
}
trap cleanup EXIT

rm -f "$dmg_path"

mkdir -p "$tmp_dir"
cp -R "$app_path" "$tmp_dir/QtScrcpy.app"
ln -s /Applications "$tmp_dir/Applications"

hdiutil create -volname "QtScrcpy" -srcfolder "$tmp_dir" -format UDZO -ov "$dmg_path"

echo
echo
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

# 恢复当前目录
cd $old_cd
exit 0
