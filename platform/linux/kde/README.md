# ChromaEngine — KDE Plasma wallpaper plugin

**Not yet tested on a real Plasma system.** This was written in a
Windows-only environment with no Linux/Plasma machine, VM, or WSL install
available, so unlike the Windows build (which was actually compiled and run
live), this is careful-but-unverified code against Plasma/Qt6's documented
wallpaper plugin APIs. Please install it on your actual KDE Plasma system,
try it, and report back anything that breaks — QML errors show up in
`journalctl --user -f` or by running `plasmashell --replace` from a
terminal while you switch wallpaper types.

## What this is

Unlike Windows (which has no OS concept of a video wallpaper, hence the
WorkerW window-injection trick in `platform/windows/`), Plasma has an actual
wallpaper plugin API: a folder of QML + a KConfigXT schema, loaded directly
by `plasmashell`. No compiled component, no installer — just files copied
into place.

- `metadata.json` — plugin identity/registration.
- `contents/config/main.xml` — the persisted settings schema (KConfigXT):
  folder path, selected file, fill mode, mute, volume, playback speed.
- `contents/ui/main.qml` — the actual wallpaper: `AnimatedImage` for GIFs,
  `MediaPlayer` + `VideoOutput` (QtMultimedia) for everything else, looped
  infinitely.
- `contents/ui/config.qml` — the page shown in System Settings › Appearance
  › Wallpaper when this plugin is selected: a folder picker plus a grid of
  the videos/GIFs in it to click and apply.

## Requirements

- Plasma 6 (Qt6). The QtMultimedia QML API changed significantly between
  Qt5 and Qt6 (`VideoOutput.source: player` became
  `MediaPlayer.videoOutput: videoOutput`), so this will **not** work
  unmodified on Plasma 5/Qt5 — that would need a small separate variant.
- Qt6 Multimedia with a working backend, i.e. `qt6-multimedia-ffmpeg` (or
  your distro's equivalent package) actually installed — without it,
  `MediaPlayer` silently fails to produce video.

## Installing

```sh
kpackagetool6 --type Plasma/Wallpaper --install platform/linux/kde
```

To update after editing files:

```sh
kpackagetool6 --type Plasma/Wallpaper --upgrade platform/linux/kde
```

(Or skip the tool and symlink/copy this directory to
`~/.local/share/plasma/wallpapers/com.chromaengine.wallpaper/` — matching
the `Id` in `metadata.json`.)

## Using it

Right-click the desktop → **Configure Desktop and Wallpaper** → change the
"Wallpaper Type" dropdown to **ChromaEngine** → pick a folder → click a
video/GIF in the grid → Apply.

## Known gaps vs. the Windows build

- **No real thumbnails.** The config page's grid shows a generic icon
  (▶ / "GIF") and the filename per item, not an actual video frame — pure
  QML has no built-in way to grab a video frame without a compiled backend
  (the Windows app's real-frame thumbnails come from `ThumbnailGenerator.cpp`,
  a C++/libmpv component). Adding real thumbnails here would mean writing a
  small compiled QML plugin, which isn't included in this pass.
- **No per-monitor targeting, brightness/contrast/saturation, flip, or
  zoom** — Plasma's wallpaper API applies one config per wallpaper instance
  (which Plasma itself lets you set per-screen/per-activity at the shell
  level), and `VideoOutput`/`AnimatedImage` don't expose the equivalent of
  mpv's video-equalizer properties or `vf` filters. Fill mode
  (Stretch/Fit/Fill), mute/volume, and playback speed are implemented.
- **GIF and video are two different QML item types** (`AnimatedImage` vs.
  `MediaPlayer`+`VideoOutput`), switched on file extension — there's no
  single playback path the way libmpv gives the Windows build.
