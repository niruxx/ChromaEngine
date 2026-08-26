# ChromaEngine

An animated wallpaper engine that plays local MP4/GIF files as the desktop
background — a Wallpaper Engine / Lively Wallpaper style app, targeting
Windows and Linux (GNOME & KDE Plasma).

## Status

Windows is fully built and has been actually run/tested (see below). A KDE
Plasma wallpaper plugin now exists at `platform/linux/kde/` — written
against Plasma/Qt6's documented APIs, but **not yet verified on a real
Plasma system** (this was built in a Windows-only environment with no
Linux/Plasma machine available; see `platform/linux/kde/README.md` for
install/test steps and known gaps). GNOME is not started.

## How it works (Windows)

There is no public Windows API for "set a video as the desktop background."
The technique used here (the same one Wallpaper Engine/Lively Wallpaper use):

1. Force Explorer to spawn a hidden `WorkerW` window that sits behind the
   desktop icons (`platform/windows/WorkerWHost.cpp`).
2. Create a borderless window sized to the monitor and reparent it into that
   `WorkerW` (`platform/windows/WallpaperWindow.cpp`).
3. Embed [libmpv](https://mpv.io/) into that window's native handle and play
   the chosen file on a loop (`render/MpvSurface.cpp`). MP4 and GIF go
   through the same mpv decode/render path.
4. A watchdog re-attaches everything if `explorer.exe` restarts and destroys
   the `WorkerW` (this happens on Explorer crashes/restarts).

Windows doesn't document (or guarantee) which of the two `WorkerW` windows -
or, on some configurations, which sibling under `Progman` directly, since
not every system even creates a second `WorkerW` - ends up in front, so the
wallpaper can end up covering the desktop icons instead of sitting behind
them depending on the exact Windows build. Both `WorkerWHost.cpp` (for the
two-separate-`WorkerW` case) and `WallpaperWindow.cpp` (for the
shares-a-parent-with-the-icons case) explicitly re-assert the correct
z-order rather than assuming Windows got it right, and re-check it
periodically since Explorer can reshuffle it during normal use. The
"Show desktop icons" checkbox in the sidebar flips this deliberately, for
users who'd rather the wallpaper cover the icons.

The app is manifested as Per-Monitor-V2 DPI aware (`platform/windows/app.manifest`),
and the wallpaper window's size comes from a direct `GetSystemMetrics` query
(`engine/src/MonitorManager.cpp`) rather than Qt's `QScreen::geometry()` —
without that, a DPI-unaware process gets handed a scaled-down logical
resolution by Windows instead of the true physical one, and the wallpaper
ends up covering only a fraction of the screen, anchored in the top-left.

A tray icon plus a Wallpaper-Engine-style library window (`ui/SettingsWindow`)
let you point at a folder of local videos/GIFs. The window has a toolbar
(Open Folder / Refresh / Play-Pause / Settings) with a custom procedurally-
drawn icon set (`ui/IconFactory.cpp` — no external image assets), a library
grid on the left, and a preview + controls sidebar on the right.

**Library grid**: tiles are rendered by a custom delegate
(`ui/LibraryItemDelegate.cpp` — rounded cards, gradient filename captions,
accent selection outline) and auto-play a short cycling sequence of real
frames rather than a static thumbnail. GIFs sample ~6 frames directly via
`QImageReader`; videos get ~6 frames grabbed via mpv's own `screenshot-to-file`
command inside a brief hidden libmpv session
(`platform/windows/ThumbnailGenerator.cpp`) — reading mpv's decoded frame
directly sidesteps relying on window-compositing/GDI capture, which turned
out to be unreliable. A shared timer in `SettingsWindow` advances every
tile's frame in lock-step, so the whole grid feels alive without a native
window per tile (not technically possible inside a virtualized list).

**Preview & sidebar**: clicking a tile only loads it into a live preview pane
(a second, always-muted mpv instance) and shows its file size/resolution —
it does *not* touch the live desktop wallpaper until you press **Set as
Wallpaper**. The sidebar also has Rename/Delete file actions, alignment
(Fill/Fit/Stretch/Center/Free with a zoom slider), horizontal/vertical flip,
brightness/contrast/saturation, and playback speed — all backed by mpv's own
properties/`vf` filters and persisted like the existing mute/volume/fit
settings.

**Multi-monitor**: the Settings dialog (gear icon) has a Displays tab listing
every real connected monitor (`MonitorManager::listMonitors()` via
`EnumDisplayMonitors`); toggling one adds/removes a wallpaper surface for
that monitor live. Its General tab has a default wallpaper folder picker,
launch-at-Windows-startup (a per-user registry Run-key toggle,
`platform/windows/StartupManager.cpp`), and whether closing the window
minimizes to the tray or exits; an Appearance tab picks the library panel's
animated background theme (None/Aurora/Starfield, `ui/AnimatedBackground.cpp`).

The library window has its own custom title bar (`ui/CustomTitleBar.cpp`)
matching the app's dark theme instead of the native Windows chrome - move,
resize-from-edges, and double-click-to-maximize are wired up via
`WM_NCHITTEST` in `SettingsWindow::nativeEvent`.

On first run (no folder configured yet) the library window opens
automatically; once a folder is set, the app starts quietly in the tray.
Settings persist to `%APPDATA%\chroma-engine\ChromaEngine\config.json`.

**A note on the thumbnail pipeline**: it originally rendered each video into
a small on-screen window with a `WS_EX_LAYERED` alpha=0 trick to keep it
invisible while grabbing frames via GDI. That trick doesn't reliably hide
mpv's GPU-presented content (flip-model swap chains often bypass
layered-window alpha blending), which showed up as videos visibly flashing
in a corner of the screen while a folder was scanned. Frame capture now goes
through mpv's own `screenshot-to-file` command instead, which reads the
decoded frame directly rather than the window's presented pixels - so the
capture window no longer needs to be composited (or even on-screen) at all,
and sits off-screen unconditionally.

## How it works (KDE Plasma)

Plasma, unlike Windows, has an actual wallpaper plugin API — no
window-injection tricks needed. `platform/linux/kde/` is a self-contained
QML + KConfigXT package (no compiled component, no build step for end
users): `contents/ui/main.qml` renders the wallpaper (`AnimatedImage` for
GIFs, `MediaPlayer`+`VideoOutput` for video, both looped), and
`contents/ui/config.qml` is the folder-and-file picker shown in System
Settings. See `platform/linux/kde/README.md` for install steps and the
current feature gaps relative to Windows (no real video thumbnails, no
brightness/contrast/flip/zoom — mpv's video-equalizer/`vf` filters don't
have a QtMultimedia equivalent).

## Current scope

Implemented (Windows): multi-monitor wallpaper targeting, local MP4/GIF
playback from a browsable folder library with animated real thumbnails and a
live preview, per-file alignment/flip/filters/speed, file rename/delete,
mute/volume, default wallpaper folder, launch at startup, close-to-tray,
animated background themes, a custom title bar, settings persistence,
explorer-restart recovery, single-instance guard.

Implemented (KDE Plasma), unverified: folder-based library with a file
picker, fill mode, mute/volume, playback speed — see
`platform/linux/kde/README.md` for what's missing relative to Windows.

Not yet implemented (planned): per-monitor *different* wallpapers on
Windows (all enabled monitors currently show the same file),
playlists/scheduling, installer packaging, and GNOME. The Windows OS-wide
accent color was deliberately left out — descoped by request in favor of
not touching system-wide theme state.

## Building (Windows)

Requirements: CMake 3.21+, a C++20 compiler (MSVC or MinGW), Qt6
(Core/Gui/Widgets), and libmpv — see [third_party/README.md](third_party/README.md)
for how to get libmpv.

```
cmake -B build -DCMAKE_PREFIX_PATH=<path to your Qt6 install> -DMPV_DIR=<path to libmpv dev package>
cmake --build build --config Release
```

The built executable is `ChromaEngine.exe`; `mpv-2.dll` is copied next to it
automatically during the build.

## Project layout

```
engine/     platform-agnostic core: config persistence, monitor enumeration, folder scanning
render/     libmpv wrapper (video + GIF playback, filters/flip/speed/zoom)
ui/         library-grid window, preferences dialog, custom icon set, tray icon, animation helpers
platform/windows/   WorkerW injection, wallpaper window(s), thumbnail grabber, startup registry, DPI manifest, app entry point
platform/linux/kde/ Plasma wallpaper plugin (QML + KConfigXT, no build step) - see its own README
third_party/        notes on fetching libmpv
```
