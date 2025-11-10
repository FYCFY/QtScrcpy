# Repository Guidelines

## Project Structure & Module Organization
The root CMake project builds the lean auto-cast runner. `QtScrcpyCore/` still
hosts the upstream streaming layer (adb helpers, server binary, encoding
plumbing). Mirroring UI lives in `QtScrcpy/ui/videoform.*`, backed by
`QtScrcpy/render/qyuvopenglwidget.*`, while the minimal status/console window
is implemented in `QtScrcpy/ui/statuswindow.*`. Automatic device discovery and
window lifecycle logic sit in `QtScrcpy/autocastcontroller.*`. Shared helpers
(configuration, mouse hooks, server-path resolution) reside in `QtScrcpy/util/`, while
`uibase/keepratiowidget.*` maintains the aspect-ratio container. Android-side
artifacts are under `QtScrcpy/QtScrcpyCore/src/third_party/`, and repo-level
configs/assets live in `config/` and `keymap/`. Packaging scripts remain in `ci/`
by platform.

## Build, Test, and Development Commands
- Clone with submodules (`git clone --recurse-submodules …`) so `QtScrcpyCore`
  pulls its dependencies.
- Generic build: `cmake -S QtScrcpy -B build -DCMAKE_PREFIX_PATH=$ENV_QT_PATH/... -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8`. Qt ≥5.12 (Qt6 on mac arm64) is required.
- Convenience scripts: `./ci/linux/build_for_linux.sh Release`, `./ci/mac/build_for_mac.sh Release arm64`, `ci/win/build_for_win.bat Release x64`.
- Packaging entry points: `./ci/linux/package_appimage.sh`, `ci/mac/package_for_mac.sh`, `ci/win/publish_for_win.bat`; run them only after populating `output/<arch>/<config>/`.

## Coding Style & Naming Conventions
`.clang-format` (WebKit-derived) enforces 4-space indentation, 160-character
limits, aligned operators, and `UseTab: Never`. Qt-facing classes prefer
UpperCamelCase, helpers lean lowerCamelCase, globals keep `g_` prefixes, and
namespaces such as `qsc` stay lower-case. Add new headers to `res/res.qrc` when
introducing assets, and let `AUTOUIC/AUTOMOC` handle UI classes.

## Testing Guidelines
There is no automated suite. Validate changes by plugging/unplugging at least
one device, ensuring `AutoCastController` starts/stops mirroring, confirming
mouse/keyboard injection inside `VideoForm`, and exercising frameless/on-top
options controlled via `config.ini`. Capture `adb logcat -s QtScrcpy` plus
console logs when debugging and describe manual scenarios when touching
`config/` or `keymap/`.

## Commit & Pull Request Guidelines
Use the existing prefixes (`feat:`, `fix:`, `chore:`, `refactor:`) and keep the
subject under ~72 chars. Every change should build on Linux, macOS, and Windows
when feasible and avoid bundling large binaries. PRs need a short summary,
linked issue (if any), and the platforms you tested; screenshots/clips are only
needed when altering `VideoForm`. Highlight any required env vars
(`ENV_QT_PATH`, `QTSCRCPY_*`) or config edits.

## Security & Configuration Tips
Avoid committing device identifiers, private keys, or real userdata under
`config/`. Only replace `scrcpy-server` inside
`QtScrcpy/QtScrcpyCore/src/third_party/` and verify upstream hashes. When
editing adb path logic, keep environment-variable overrides opt-in and ensure
`Config::getInstance()` respects user-supplied values before falling back to
defaults.
