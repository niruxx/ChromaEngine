# colorfy-engine

An animated wallpaper engine that plays local MP4/GIF files as the desktop
background — a Wallpaper Engine / Lively Wallpaper style app, targeting
Windows and Linux (GNOME & KDE Plasma).

## Status

Windows is the platform being built out first (see below); GNOME and KDE
Plasma integrations are planned but not implemented yet.

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

The app is manifested as Per-Monitor-V2 DPI aware (`platform/windows/app.manifest`),
and the wallpaper window's size comes from a direct `GetSystemMetrics` query
(`engine/src/MonitorManager.cpp`) rather than Qt's `QScreen::geometry()` —
without that, a DPI-unaware process gets handed a scaled-down logical
resolution by Windows instead of the true physical one, and the wallpaper
ends up covering only a fraction of the screen, anchored in the top-left.

A tray icon plus a Wallpaper-Engine-style library window (`ui/SettingsWindow`)
let you point at a folder of local videos/GIFs. The window has a toolbar
(Open Folder / Refresh / Play-Pause), a grid of clickable preview tiles
rendered by a custom delegate (`ui/LibraryItemDelegate.cpp` — rounded cards,
gradient filename captions, accent selection outline), and a live preview
pane on the right: a second, always-muted mpv instance that actually plays
the selected file, not just a static thumbnail. GIF tiles show a real first
frame immediately; video tiles get a real frame grabbed via a brief hidden
libmpv instance (`platform/windows/ThumbnailGenerator.cpp`), falling back to
a placeholder icon rather than a misleading black tile if that frame turns
out to be genuinely dark. Mute/volume and fit mode (Fill/Fit/Stretch/Center)
live under the preview. On first run (no folder configured yet) the library
window opens automatically; once a folder is set, the app starts quietly in
the tray. Settings persist to
`%APPDATA%\colorfy-engine\ColorfyEngine\config.json`.

## Current scope

Implemented: single (primary) monitor, local MP4/GIF playback from a
browsable folder library with real thumbnails, mute/volume, fit mode,
settings persistence, explorer-restart recovery, single-instance guard.

Not yet implemented (planned): per-monitor wallpaper assignment on
multi-monitor setups, run-at-login, playlists/scheduling, installer
packaging, and the GNOME (Shell extension) and KDE Plasma (native wallpaper
QML plugin) integrations.

## Building (Windows)

Requirements: CMake 3.21+, a C++20 compiler (MSVC or MinGW), Qt6
(Core/Gui/Widgets), and libmpv — see [third_party/README.md](third_party/README.md)
for how to get libmpv.

```
cmake -B build -DCMAKE_PREFIX_PATH=<path to your Qt6 install> -DMPV_DIR=<path to libmpv dev package>
cmake --build build --config Release
```

The built executable is `ColorfyEngine.exe`; `mpv-2.dll` is copied next to it
automatically during the build.

## Project layout

```
engine/     platform-agnostic core: config persistence, monitor geometry, folder scanning
render/     libmpv wrapper (video + GIF playback)
ui/         library-grid settings window (toolbar, custom tile delegate, live preview) + tray icon
platform/windows/   WorkerW injection, wallpaper window, thumbnail grabber, DPI manifest, app entry point
third_party/        notes on fetching libmpv
```
