# 获取绝对路径，保证其他目录执行此脚本依然正确
{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output
# 设置当前目录，cd的目录影响接下来执行程序的工作目录
old_cd=$(pwd)
cd $(dirname "$0")

echo
echo
echo ---------------------------------------------------------------
echo prepare python environment
echo ---------------------------------------------------------------

python_bin=${PYTHON_BIN:-python3}
if ! command -v "$python_bin" >/dev/null 2>&1; then
    python_bin=python
fi

if ! "$python_bin" -c "import dmgbuild" >/dev/null 2>&1; then
    "$python_bin" -m pip install --disable-pip-version-check --no-cache-dir 'dmgbuild==1.4.2'
    if [ $? -ne 0 ] ;then
        echo "failed to install dmgbuild"
        exit 1
    fi
fi

echo
echo
echo ---------------------------------------------------------------
echo create package
echo ---------------------------------------------------------------

"$python_bin" $script_path/package/package.py
if [ $? -ne 0 ] ;then
    echo "create package failed"
    exit 1
fi

# 恢复当前目录
cd $old_cd
exit 0
