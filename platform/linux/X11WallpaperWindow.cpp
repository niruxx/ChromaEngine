#include "X11WallpaperWindow.h"

#include <QDebug>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

// Xlib's headers #define plain identifiers like None/Bool/True/False/Status
// as macros, which can silently mangle unrelated code below that happens to
// use the same names (Qt has several). This file only uses False (for
// XInternAtom's only_if_exists) alongside Xlib's own types, so it's safe as
// written - but any Qt-facing code added below must avoid those five names.
namespace colorfy {

namespace {

// Xlib's default error handler for a protocol error (BadWindow, BadGC, ...)
// prints one message and then calls exit() - killing the whole process, not
// just whatever triggered it. Confirmed live: a transient BadWindow/BadGC
// pair (most likely mpv's own X11 rendering resources for the "wid" this
// window hands it, momentarily racing this window's placement calls below)
// took the entire app down mid-playback despite being otherwise harmless.
// Logging instead of aborting is the standard fix for any long-running app
// that touches Xlib directly.
int handleXError(Display* display, XErrorEvent* event)
{
    char message[256];
    XGetErrorText(display, event->error_code, message, sizeof(message));
    qWarning() << "X11 error (non-fatal):" << message << "request" << event->request_code << "resource"
               << event->resourceid;
    return 0;
}

// A private Xlib connection, independent of Qt's own XCB one. Xlib window
// IDs are valid across any connection to the same X server, so this avoids
// needing Qt6's QNativeInterface plumbing just to reparent/restack a window
// we already own via winId() - only used for the occasional structural call
// below (reparent, resize, lower, one property change), never for reading
// events, so sharing a connection with Qt's event loop isn't a concern.
Display* nativeDisplay()
{
    static Display* display = [] {
        Display* d = XOpenDisplay(nullptr);
        if (d)
            XSetErrorHandler(handleXError);
        return d;
    }();
    return display;
}

} // namespace

void installX11ErrorHandler()
{
    if (nativeDisplay())
        XSetErrorHandler(handleXError);
}

X11WallpaperWindow::X11WallpaperWindow(const QRect& geometry, QWidget* parent)
    : QWidget(parent)
    , m_geometry(geometry)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // BypassWindowManagerHint makes Qt create this as an override-redirect
    // X11 window, so the window manager never reparents it into a decorated
    // frame or otherwise manages its stacking - XLowerWindow below then has
    // full, uncontested control over where it sits.
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus | Qt::BypassWindowManagerHint);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    setGeometry(m_geometry);

    // Force native X11 window creation now so winId() is valid below.
    winId();
    show();

    reassertPlacement();
}

void* X11WallpaperWindow::nativeHandle() const
{
    return reinterpret_cast<void*>(winId());
}

void X11WallpaperWindow::reassertPlacement()
{
    Display* display = nativeDisplay();
    if (!display)
        return;

    const auto window = static_cast<Window>(winId());
    const int screenNumber = DefaultScreen(display);
    const Window root = RootWindow(display, screenNumber);

    // Advisory hint for window managers/compositors that do inspect
    // override-redirect windows (harmless if ignored otherwise).
    const Atom windowTypeAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    const Atom desktopTypeAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(display, window, windowTypeAtom, XA_ATOM, 32, PropModeReplace,
                     reinterpret_cast<const unsigned char*>(&desktopTypeAtom), 1);

    // Being exactly monitor-sized and override-redirect is precisely the
    // shape compositing window managers look for to "unredirect" a window -
    // an optimization (meant for fullscreen games/video players) that
    // paints it directly to the screen outside the normal composited
    // stacking order entirely. Confirmed live on xfwm4: without opting out,
    // this window ended up covering the whole screen - including the panel
    // and desktop icons - regardless of XLowerWindow, because unredirected
    // painting doesn't go through the stacking order compositing normally
    // enforces. _NET_WM_BYPASS_COMPOSITOR=2 is the de-facto EWMH hint (honored
    // by xfwm4, KWin, Mutter, and Compiz) for "always composite this window,
    // never unredirect it" - the opposite of what most fullscreen apps want,
    // which is exactly what a wallpaper needs.
    const Atom bypassCompositorAtom = XInternAtom(display, "_NET_WM_BYPASS_COMPOSITOR", False);
    const long neverBypass = 2;
    XChangeProperty(display, window, bypassCompositorAtom, XA_CARDINAL, 32, PropModeReplace,
                     reinterpret_cast<const unsigned char*>(&neverBypass), 1);

    // Reparenting is normally redundant once BypassWindowManagerHint has
    // already made this a direct child of root, but costs nothing and
    // guards against any platform-plugin quirk that parents it elsewhere.
    XReparentWindow(display, window, root, m_geometry.x(), m_geometry.y());
    XResizeWindow(display, window, static_cast<unsigned int>(m_geometry.width()),
                  static_cast<unsigned int>(m_geometry.height()));

    XLowerWindow(display, window);
    XFlush(display);
}

void X11WallpaperWindow::lowerToBottom()
{
    Display* display = nativeDisplay();
    if (!display)
        return;

    // See installX11ErrorHandler()'s comment (X11WallpaperWindow.h): mpv's
    // own GL/GLX context setup has been observed to silently swap out the
    // process-wide Xlib error handler without restoring it. Reclaiming it
    // here piggybacks on the watchdog's existing tick rather than needing a
    // separate timer, so it's never unclaimed for more than one tick.
    installX11ErrorHandler();

    // Root's children stack strictly by z-order, so as long as we're pushed
    // to the very bottom every time this runs, whatever the desktop shell
    // draws (icons, its own background) stays on top of us.
    XLowerWindow(display, static_cast<Window>(winId()));
    XFlush(display);
}

} // namespace colorfy
