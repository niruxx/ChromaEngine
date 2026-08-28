#pragma once

#include <QRect>
#include <QWidget>

namespace colorfy {

// (Re-)installs the process-wide Xlib error handler that keeps a stray X
// protocol error (BadWindow/BadGC, etc.) from taking down the whole app -
// see X11WallpaperWindow.cpp for why this needs to be reclaimed after
// anything that sets up its own GL/GLX context (mpv, most notably: its own
// context setup has been observed to silently swap this out without
// restoring it). Call after constructing each MpvSurface, in addition to
// the periodic reinstall X11WallpaperWindow::lowerToBottom() already does.
void installX11ErrorHandler();

// A borderless, override-redirect X11 window sized to a monitor, forced to
// the very bottom of the root window's stacking order so it renders behind
// every other window - including the desktop-icon window XFCE's xfdesktop
// and MATE's Caja/Marco desktop manager draw on top of the root.
//
// This is the same "reparent into root, then lower" technique xwinwrap and
// mpvpaper use, chosen because neither XFCE nor MATE expose a wallpaper
// plugin API the way KDE Plasma does (see platform/linux/kde/) - there is no
// desktop-specific hook to target, so this works generically against any
// EWMH-aware X11 window manager instead. It needs a real X11 session:
// Wayland compositors (GNOME Shell's default session, for instance) don't
// expose the root-window stacking model this depends on - see the
// platformName() check in main_linux.cpp.
class X11WallpaperWindow : public QWidget {
    Q_OBJECT
public:
    explicit X11WallpaperWindow(const QRect& geometry, QWidget* parent = nullptr);

    void* nativeHandle() const;

    // Full one-time setup: the desktop-type hint, reparent into root, resize
    // to the monitor's geometry, and lower to the bottom of the stack.
    // Called once from the constructor.
    void reassertPlacement();

    // Just the "lower to the bottom of the stack" part of the above, safe to
    // call on every tick of the watchdog timer in main_linux.cpp - a
    // desktop shell restart (xfdesktop, most notably) can re-raise its own
    // desktop window above ours, so this needs to run periodically. Unlike
    // reassertPlacement(), it doesn't touch XReparentWindow/XResizeWindow:
    // those are only needed once, and doing them repeatedly against a
    // window with an active GL surface bound to it (mpv's rendering
    // context) turned out to be the cause of a live X11 protocol error
    // (BadWindow/BadGC) that killed the whole process - see this class's
    // .cpp for the fix that made that non-fatal regardless, but there's no
    // reason to keep doing the disruptive part on every tick anyway.
    void lowerToBottom();

private:
    QRect m_geometry;
};

} // namespace colorfy
