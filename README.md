# ChromaEngine

An animated wallpaper engine that plays local MP4/GIF files as the desktop
background — a Wallpaper Engine / Lively Wallpaper style app, targeting
Windows and Linux (GNOME & KDE Plasma).

## Status

Windows is fully built and has been actually run/tested (see below). Linux
has two independent wallpaper paths:

- `platform/linux/kde/` — a KDE Plasma wallpaper plugin (QML + KConfigXT,
  written against Plasma/Qt6's documented APIs) for users on Plasma. See
  `platform/linux/kde/README.md` for install steps and known gaps. **Not yet
  verified on a real Plasma system.**
- `platform/linux/` (this directory's siblings, building the same
  `ChromaEngine` executable as Windows) — a generic X11 build for desktop
  environments with no wallpaper plugin API of their own, namely **XFCE and
  MATE**. It embeds libmpv the same way the Windows build does, just behind
  an X11-specific window placement trick instead of the WorkerW one - see
  "How it works (XFCE / MATE / other X11 desktops)" below. **Built and
  actually run on a real XFCE (Fedora 44, X11 session) machine** - the
  library/tray/preview/thumbnail UI all confirmed working live. Three real
  bugs only surfaced this way and are now fixed: an unhandled X11 protocol
  error that could kill the whole process (`X11WallpaperWindow.cpp`'s
  `handleXError`), xfwm4's compositor "unredirecting" the monitor-sized
  wallpaper window and covering the entire screen with it instead of placing
  it behind everything (`_NET_WM_BYPASS_COMPOSITOR`, same file), and mpv's
  default GPU-accelerated rendering path being unreliable on that
  particular GPU-less test machine (`DRI3`/`VDPAU` both unavailable there) -
  a new **"Force software rendering"** preference (General tab) switches mpv
  to its plain-Xlib `vo=x11` output, which needs no GL/EGL context at all;
  see "How it works" below for what it trades off. Two gaps remain open,
  both specific to that test machine rather than confirmed as real bugs:
  occasional crashes even with software rendering on (a different, more
  specific X11/MIT-SHM error than the one the general handler above already
  covers), and - a structural finding, not really an app bug - **xfdesktop
  draws its desktop background and icons as a single opaque window covering
  the whole screen**, so on a system configured that way nothing placed
  behind it (by this app or by any similar tool) will be visible until
  xfdesktop's own background is set to "None" via XFCE's Desktop Settings.
  All of this needs checking on real, non-virtualized GPU hardware and a
  default XFCE background setup to tell how much is this environment's
  limitation versus a real bug - please report back either way.

GNOME is not started - it defaults to a Wayland session, which the X11 path
above doesn't support (see that section for why), and has no wallpaper
plugin API like Plasma's to target instead.

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

## How it works (XFCE / MATE / other X11 desktops)

Neither XFCE nor MATE expose anything like Plasma's wallpaper plugin API, so
`platform/linux/` instead builds the same kind of standalone app as Windows —
same `engine`/`render`/`ui` code, same tray icon and library window, same
libmpv-based `MpvSurface` — with an X11-specific technique standing in for
Windows' WorkerW injection:

1. `X11WallpaperWindow` creates a borderless, override-redirect window sized
   to a monitor (`Qt::BypassWindowManagerHint`, so the window manager never
   reparents or manages it) and reparents it directly into the X11 root
   window, then calls `XLowerWindow` to force it to the very bottom of the
   root's stacking order.
2. libmpv embeds into that window the same way it embeds into an `HWND` on
   Windows — `MpvSurface` just sets mpv's `wid` option to the window's X11
   `Window` ID instead of a Windows handle.
3. A periodic watchdog in `main_linux.cpp` re-lowers each wallpaper window
   every couple of seconds, since a desktop shell restart (xfdesktop, most
   notably) can re-raise its own desktop-icon window above ours otherwise.

This is the same "reparent into root, lower to the bottom" approach
`xwinwrap` and `mpvpaper` use, and it works the same way against any
EWMH-compliant X11 window manager rather than needing a desktop-specific
hook — which is also why it's not GNOME-specific and not tied to XFCE/MATE
by name, just by the fact that both default to an X11 session today.

**It needs a real X11 session.** Wayland compositors don't expose a root
window or the override-redirect stacking model this depends on — running
under Wayland (GNOME Shell's default, for instance), the app detects this at
startup (`QGuiApplication::platformName() != "xcb"`) and warns that the
library/tray still work but the wallpaper itself won't display. XFCE and
MATE both default to X11 sessions as of writing, so this isn't a practical
limitation for either.

**It also needs xfdesktop (or your desktop manager's equivalent) to not be
painting its own opaque background over the whole screen.** xfdesktop draws
the desktop background and icons together as a single window, not as two
separate layers - if that window covers the full screen (XFCE's default),
nothing placed behind it in the stacking order is visible, no matter how
correctly it's positioned. This isn't specific to this app - it's the same
constraint every reparent-into-root wallpaper tool (`xwinwrap`, `mpvpaper`,
etc.) runs into on XFCE. If the wallpaper doesn't appear, check XFCE's own
Desktop Settings and set the background to "None" first, then let
ChromaEngine draw underneath. (Confirmed as the cause on one test machine;
not yet confirmed whether MATE's Caja/Marco desktop manager has the same
behavior or something more cooperative.)

**Rendering**: mpv's default video output needs a working GPU context
(GL/EGL) to embed into the given window. On hardware where that's
missing, broken, or flaky - virtual machines and some remote desktops,
most commonly - it can fail to embed at all and fall back to a separate
top-level mpv window instead of becoming the wallpaper, or behave
unreliably in other ways. The **"Force software rendering"** checkbox in
Preferences → General switches mpv to its `vo=x11` output instead, which
needs no GPU context whatsoever (confirmed live: eliminates the
DRI3/VDPAU driver warnings entirely) at the cost of higher CPU usage and no
hardware-accelerated decode. Takes effect the next time the app starts, same
as the memory limit setting above it.

**Known gaps relative to Windows**: no equivalent of "show/hide desktop
icons behind the wallpaper" (that checkbox is hidden on Linux — there's no
cross-desktop-environment way to reorder against XFCE's/MATE's own
desktop-icon window the way Windows' WorkerW-based build can); the
clock/calendar/battery overlay's Bluetooth battery readout depends on
BlueZ's `org.bluez.Battery1` D-Bus interface being populated for a device
(true for most modern BLE devices bluetoothd already tracks, but not
guaranteed for every device/driver combination the way Windows' own
Settings-backed battery value is).

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

Implemented (Linux/X11 — XFCE, MATE, and other EWMH window managers), built
and run live on real XFCE: everything the Windows build has except
per-monitor *different* wallpapers (same limitation as Windows, see below),
the OS-wide accent color (same descope as Windows), and "show/hide desktop
icons behind the wallpaper" — see "How it works (XFCE / MATE / other X11
desktops)" above for what's missing and why, including the one open item
from live testing (mpv embedding on GPU-less machines).

Not yet implemented (planned): per-monitor *different* wallpapers (all
enabled monitors currently show the same file, on every platform),
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

## Building (Linux — XFCE, MATE, other X11 desktops)

Requirements: CMake 3.21+, a C++20 compiler (GCC or Clang), Qt6
(Core/Gui/Widgets/DBus), libmpv, and Xlib development headers. On Debian/
Ubuntu:

```
sudo apt install build-essential cmake qt6-base-dev libqt6dbus6 qt6-base-dev-tools \
                  libmpv-dev libx11-dev
```

On Fedora:

```
sudo dnf install cmake gcc-c++ qt6-qtbase-devel mpv-libs-devel libX11-devel
```

Then, from the repo root:

```
cmake -B build
cmake --build build
```

No `-DMPV_DIR` needed here — unlike Windows, Linux distros package libmpv's
`pkg-config` file directly, and `render/CMakeLists.txt` finds it that way.
The built executable is `build/platform/linux/ChromaEngine`; running
`cmake --install build` installs it plus a `.desktop` launcher entry
(`platform/linux/chroma-engine.desktop`).

KDE Plasma users: install `platform/linux/kde/` instead (see its own
README) for the native wallpaper-plugin experience — this X11 build still
works under a Plasma X11 session as a fallback, just without Plasma's own
per-screen/per-activity wallpaper configuration UI around it.

## Project layout

```
engine/     platform-agnostic core: config persistence, monitor enumeration, folder scanning
render/     libmpv wrapper (video + GIF playback, filters/flip/speed/zoom)
ui/         library-grid window, preferences dialog, custom icon set, tray icon, animation helpers
platform/windows/   WorkerW injection, wallpaper window(s), thumbnail grabber, startup registry, DPI manifest, app entry point
platform/linux/     X11 wallpaper window (reparent-into-root + lower), thumbnail grabber, XDG autostart, BlueZ battery reader, app entry point
platform/linux/kde/ Plasma wallpaper plugin (QML + KConfigXT, no build step) - see its own README
third_party/        notes on fetching libmpv
```
