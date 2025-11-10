# QtScrcpy（自动投屏版）

本分支只保留 QtScrcpy 的核心投屏能力：程序在后台监控 `adb devices`，
一旦检测到授权设备就自动创建一个 `VideoForm` 窗口进行渲染，无需任何按钮
或主控制面板。所有群控、音频、游戏键位可视化和其它复杂 UI 都已移除。

## 目录概览

- `QtScrcpyCore/`：原始 scrcpy 核心，包含 adb 通讯、服务端和解码逻辑。
- `QtScrcpy/ui/videoform.*`：唯一的前端窗口，负责 OpenGL 渲染、键鼠注入。
- `QtScrcpy/render/qyuvopenglwidget.*`：YUV→RGB 着色器。
- `QtScrcpy/autocastcontroller.*`：自动检测 ADB 设备并启动/停止 `VideoForm`。
- `QtScrcpy/util/`：配置加载、鼠标 Hook、服务器路径解析等工具。
- `config/`：`config.ini` 与 `userdata.ini`（比特率、分辨率、窗口状态等）。
- `keymap/`：可选自定义键位脚本，仍由核心识别。

## 构建

需要 Qt ≥ 5.12（macOS arm64 可使用 Qt 6）以及支持 C++17 的编译器：

```bash
git clone --recurse-submodules https://github.com/barry-ran/QtScrcpy.git
cd QtScrcpy
cmake -S QtScrcpy -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$ENV_QT_PATH/clang_64/lib/cmake/Qt5
cmake --build build -j8
```

也可以参考 `ci/linux|mac|win` 目录中的打包脚本。

## 使用方式

1. 确认 `adb devices` 能看到设备，或在运行前设置好 `QTSCRCPY_ADB_PATH`。
2. 直接启动编译产物（位于 `output/<arch>/<config>/QtScrcpy`）。会弹出一个
   简单的监控窗口，用于显示 ADB 设备列表与实时日志。程序默认
   `setQuitOnLastWindowClosed(false)`，因此关闭某个投屏窗口只会断开该设备；
   关闭监控窗口或手动结束进程，才能完全退出。
3. 可选环境变量：
   - `QTSCRCPY_ADB_PATH`：自定义 adb 路径。
   - `QTSCRCPY_SERVER_PATH`：指定 `scrcpy-server` 位置。
   - `QTSCRCPY_CONFIG_PATH`：重定向配置目录。

若需调整比特率、最大分辨率、是否置顶、皮肤等，请直接编辑
`config/config.ini` 与同目录下的 `userdata.ini`。如果想隐藏屏幕旁的功能
按钮（返回 / Home / 最近任务 / 电源），可以把 `userdata.ini` 中的
`showToolbar` 设置为 `false`。

## 致谢

- 自动检测思路来自作者之前的 Windows 小工具
  `scrcpy_FYC/scrcpy.c`。
- 本项目依赖 [QtScrcpy](https://github.com/barry-ran/QtScrcpy) 与
  [scrcpy](https://github.com/Genymobile/scrcpy) 的核心能力，感谢原作者的贡献。
