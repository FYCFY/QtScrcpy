# QtScrcpy (Auto-Cast Edition)

This fork strips the original QtScrcpy desktop control panel and keeps only the
lightweight rendering pipeline. A single `VideoForm` window is spawned for every
ADB device that appears, so you can plug in a phone and start mirroring
immediately—no buttons, no scripts, no manual setup.

## Repository Layout

- `QtScrcpyCore/` – the upstream streaming core (adb bridge, encoder, server).
- `QtScrcpy/ui/videoform.*` – the only remaining widget; it renders YUV frames,
  forwards keyboard/mouse events, and exposes window shortcuts.
- `QtScrcpy/render/qyuvopenglwidget.*` – OpenGL YUV→RGB shader used by
  `VideoForm`.
- `QtScrcpy/autocastcontroller.*` – polls `adb devices`, launches/tears down
  mirrors, and keeps windows on top when requested.
- `QtScrcpy/util/` – configuration loader, mouse-hook helpers, server-path
  resolver, etc.
- `config/` – default `config.ini` plus user-tunable options (bitrate, max size,
  codec).
- `keymap/` – optional custom key mappings that are still understood by the
  core.

All other GUI modules (dialogs, toolbars, audio forwarding, group control) have
been removed.

## Build

Qt 5.12+ (or Qt 6 on macOS arm64) and a C++17 compiler are required. Clone with
submodules so `QtScrcpyCore` is available:

```bash
git clone --recurse-submodules https://github.com/barry-ran/QtScrcpy.git
cd QtScrcpy
cmake -S QtScrcpy -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$ENV_QT_PATH/clang_64/lib/cmake/Qt5
cmake --build build -j8
```

Platform scripts still exist under `ci/<platform>/build_for_*.sh` if you prefer a
wrapper.

## Usage

1. Make sure `adb` sees your device (`adb devices`). The app will also start the
   daemon if necessary.
2. Launch the built binary (`QtScrcpy` inside `output/<arch>/<config>/`). A small
   monitor window will appear, showing ADB status logs and the list of detected
   devices. The process keeps running in the background
   (`QApplication::setQuitOnLastWindowClosed(false)`), so closing an individual
   mirror window only disconnects that serial; close the monitor window (or halt
   the process) to exit completely.
3. Optional environment overrides:
   - `QTSCRCPY_ADB_PATH` – path to your adb executable (otherwise falls back to
     the bundled copy).
   - `QTSCRCPY_SERVER_PATH` – absolute path to `scrcpy-server`.
   - `QTSCRCPY_CONFIG_PATH` – custom config location.

Window geometry, FPS overlays, frameless mode, bitrate, etc. are read from
`config/config.ini` and `userdata.ini`; edit those files to change defaults.

## Credits

- Auto-detection is inspired by the author's earlier Windows helper
  (`scrcpy_FYC/scrcpy.c`).
- Streaming and decoding are powered by the upstream
  [QtScrcpy](https://github.com/barry-ran/QtScrcpy) and
  [scrcpy](https://github.com/Genymobile/scrcpy) projects—many thanks to both.
