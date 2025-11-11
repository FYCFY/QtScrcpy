# Repository Guidelines

## Project Structure & Module Organization
The root CMake target under `QtScrcpy/` builds the auto-cast runner that spawns one `VideoForm` per detected device. Streaming helpers, adb plumbing, and the embedded `scrcpy-server` live in `QtScrcpy/QtScrcpyCore/`. The UI code is trimmed to `QtScrcpy/ui/videoform.*`, rendered through `QtScrcpy/render/qyuvopenglwidget.*`, while auto-discovery logic is in `QtScrcpy/autocastcontroller.*`. Shared utilities (`QtScrcpy/util/`), aspect-ratio helpers (`uibase/keepratiowidget.*`), and configuration/keymaps (`config/`, `keymap/`) round out the workspace; packaging scripts are under `ci/<platform>/`.

## Build, Test, and Development Commands
- `cmake -S QtScrcpy -B build -DCMAKE_PREFIX_PATH=$ENV_QT_PATH/... -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8`: configure and compile against Qt 5.12+ (Qt 6 on mac arm64).
- `./ci/mac/build_for_mac.sh Release arm64` / `./ci/linux/build_for_linux.sh Release`: platform wrappers that set toolchains and output paths.
- `./ci/mac/package_for_mac.sh` (or equivalents in `ci/linux`, `ci/win`): bundle artifacts from `output/<arch>/<config>/` once binaries exist.

## Coding Style & Naming Conventions
`.clang-format` enforces 4-space indentation, 160-column width, and no tabs; run `clang-format -i` on touched C++ sources. Qt-facing classes stay UpperCamelCase, helpers prefer lowerCamelCase, globals keep `g_` prefixes, and namespaces such as `qsc` remain lowercase. Add new headers or assets to `QtScrcpy/res/res.qrc` so the build picks them up automatically.

## Testing Guidelines
There is no automated suite; validate by hand. Plug/unplug at least one device and confirm `AutoCastController` spawns and tears down `VideoForm` instances, exercise keyboard/mouse injection inside the mirrored window, and toggle frameless/on-top options via `config/config.ini`. Capture `adb logcat -s QtScrcpy` together with console output when diagnosing regressions or modifying config/keymap behavior.

## Commit & Pull Request Guidelines
Follow the existing prefixes (`feat:`, `fix:`, `chore:`, `refactor:`) and keep subjects under ~72 characters. Each PR should mention the tested platforms, summarize the change, link any tracked issue, and include screenshots only when UI visible in `VideoForm` changes. Avoid bundling large binaries and ensure code still builds on Linux, macOS, and Windows before requesting review.

## Security & Configuration Tips
Do not commit device identifiers, keys, or personal configs under `config/`. Only replace `scrcpy-server` inside `QtScrcpy/QtScrcpyCore/src/third_party/` after verifying upstream hashes. `Config::getInstance()` honors user overrides (`QTSCRCPY_*` variables, `config.ini`)—keep new logic opt-in and prefer environment variables before falling back to bundled defaults.
